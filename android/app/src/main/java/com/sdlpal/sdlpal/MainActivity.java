/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

package com.sdlpal.sdlpal;

import android.Manifest;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.*;
import androidx.annotation.NonNull;
import android.app.Activity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.provider.Settings;
import android.net.Uri;
import android.util.Log;
import android.provider.DocumentsContract;
import android.content.ContentResolver;
import androidx.documentfile.provider.DocumentFile;
import androidx.activity.result.*;
import androidx.activity.result.contract.*;

import java.io.*;
import java.util.Locale;
import java.util.concurrent.ConcurrentHashMap;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("SDL3");
        System.loadLibrary("main");
    }

    private static MainActivity mSingleton;
    private ActivityResultLauncher<Intent> permissionRequestLauncher;

    private static final String TAG = "sdlpal-debug";

    private static Uri docTreeUri = null;
    private static String basePath = "";
    private static String dataPath = "";
    private static String cachePath = "";

    private static final int MAX_CACHE_SIZE = 200;
    private static final Object NULL_MARKER = new Object(); // 用于标记不存在的占位符
    private static final ConcurrentHashMap<String, Object> sSegmentCache = new ConcurrentHashMap<>();

    public static String getBasePath() {
        return basePath;
    }

    private static void setBasePath(String basepath) {
        SetAppPath(basepath, dataPath, cachePath);
    }

    public static native void setAppPath(String basepath, String datapath, String cachepath);
    public static void SetAppPath(String basepath, String datapath, String cachepath)
    {
        basePath = basepath;
        dataPath = datapath;
        cachePath = cachepath;
        setAppPath(basePath, dataPath, cachePath);
    }

    public static boolean crashed = false;
    public static boolean blocked = false;

    private final AppCompatActivity mActivity = this;

    interface RequestForPermissions {
        void request();
    }

    private void alertUser(int id, final RequestForPermissions req) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage(id);
        builder.setCancelable(false);
        builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                req.request();
            }
        });
        builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                System.exit(1);
            }
        });
        builder.create().show();
    }

    private static int getFileDescriptorFromUri(Uri data, String mode) {
        try {
            ContentResolver cr = mSingleton.getApplicationContext().getContentResolver();
            ParcelFileDescriptor inputPFD = cr.openFileDescriptor(data, mode);
            if (inputPFD == null) {
                Log.e(TAG, "getFileDescriptorFromUri: Failed to get parcel file descriptor ");
                return -1;
            }
            return inputPFD.detachFd();
        } catch (Exception e) {
            Log.w(TAG, "getFileDescriptorFromUri:" + e.toString());
        }
        return -1;
    }

    public static Uri getDocumentUriFromPath(String path) {
        try {
            if (basePath.isEmpty())
                throw new Exception("basePath is empty");

            String relativePath = toRelativePath(path);
            if (relativePath == null) {
                throw new Exception("Path is outside basePath");
            }

            Context ctx = mSingleton.getApplicationContext();
            DocumentFile current = DocumentFile.fromTreeUri(ctx, docTreeUri);
            if (current == null || !current.exists()) {
                throw new Exception("Tree root not found");
            }

            String[] segments = relativePath.split("/");
            for (String segment : segments) {
                if (segment.isEmpty()) continue;

                DocumentFile next = findChildIgnoreCase(current, segment);
                if (next == null) {
                    return null; // 路径中某一段不存在
                }
                current = next;
            }
            return current.getUri();
        } catch (Exception e) {
            Log.w(TAG, "Exception: getDocumentUriFromPath: " + e.toString());
            return null;
        }
    }

    /**
     * 在指定父目录中查找忽略大小写匹配的子文件/目录。
     * 每个目录只会遍历一次，并将该目录下所有子项缓存。
     */
    private static DocumentFile findChildIgnoreCase(DocumentFile parent, String targetName) {
        if (parent == null || targetName == null) return null;

        String parentUri = parent.getUri().toString();
        String targetLower = targetName.toLowerCase(Locale.ROOT);
        String targetKey = parentUri + "|" + targetLower;

        // 1. 尝试从缓存获取
        Object cached = sSegmentCache.get(targetKey);
        if (cached != null) {
            if (cached == NULL_MARKER) {
                return null; // 负缓存命中
            } else if (cached instanceof DocumentFile) {
                DocumentFile df = (DocumentFile) cached;
                // 可选验证：文件是否仍然存在（防止外部删除导致缓存脏数据）
                if (df.exists()) {
                    return df;
                } else {
                    // 文件已不存在，移除缓存条目
                    sSegmentCache.remove(targetKey);
                }
            }
        }

        // 2. 缓存未命中或已失效，需要遍历目录
        // 使用父目录URI作为锁，避免并发重复遍历同一目录
        synchronized (parentUri.intern()) {
            // 双重检查：防止在等待锁期间其他线程已填充缓存
            cached = sSegmentCache.get(targetKey);
            if (cached != null) {
                if (cached == NULL_MARKER) return null;
                if (cached instanceof DocumentFile) {
                    DocumentFile df = (DocumentFile) cached;
                    if (df.exists()) return df;
                    else sSegmentCache.remove(targetKey);
                }
            }

            DocumentFile[] children = parent.listFiles();
            if (children == null) {
                // 无法列出子项，存入负缓存
                sSegmentCache.put(targetKey, NULL_MARKER);
                return null;
            }

            // 遍历所有子项，为每个子项建立缓存
            for (DocumentFile child : children) {
                String name = child.getName();
                if (name != null) {
                    String childKey = parentUri + "|" + name.toLowerCase(Locale.ROOT);
                    sSegmentCache.put(childKey, child);
                }
            }

            // 再次从缓存获取目标
            Object finalCached = sSegmentCache.get(targetKey);
            if (finalCached instanceof DocumentFile) {
                return (DocumentFile) finalCached;
            } else {
                // 目标确实不存在，存入负缓存
                sSegmentCache.put(targetKey, NULL_MARKER);
                return null;
            }
        }
    }

    /**
     * 将完整文件系统路径转换为相对于树根的路径，并进行规范化
     */
    private static String toRelativePath(String fullPath) {
        try {
            String canonicalFull = new File(fullPath).getCanonicalPath();
            String canonicalBase = new File(basePath).getCanonicalPath();
            if (!canonicalFull.startsWith(canonicalBase)) return null;
            String relative = canonicalFull.substring(canonicalBase.length());
            if (relative.startsWith("/") || relative.startsWith(File.separator)) {
                relative = relative.substring(1);
            }
            return relative;
        } catch (IOException e) {
            return null;
        }
    }
    public static boolean isExternalStorageDocument(Uri uri) {
        return "com.android.externalstorage.documents".equals(uri.getAuthority());
    }

    public static String getPath(final Uri uri) {
        Context context = mSingleton.getApplicationContext();
        // DocumentProvider
        if (DocumentsContract.isTreeUri(uri)) {
            if (isExternalStorageDocument(uri)) {
                final String docId = DocumentsContract.getTreeDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];
                if ("primary".equalsIgnoreCase(type)) {
                    return Environment.getExternalStorageDirectory() + "/" + split[1];
                }else{
                    return Environment.getExternalStorageDirectory().getPath().replace("emulated/0",type) + "/" + split[1];
                }
            }
        }else
        if (DocumentsContract.isDocumentUri(context, uri)) {
            Log.w(TAG, "WARNIGN: NEVER SHOULD BE HERE, getPath should only be called with tree uri, but got document uri:" + uri.toString());
            // ExternalStorageProvider
            if (isExternalStorageDocument(uri)) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];

                if ("primary".equalsIgnoreCase(type)) {
                    return Environment.getExternalStorageDirectory() + "/" + split[1];
                }else{
                    return Environment.getExternalStorageDirectory().getPath().replace("emulated/0",type) + "/" + split[1];
                }
            }
        }

        return "";
    }

    public static int SAF_access(String path, int mode) {
        if( path.startsWith("/data/") ) {
            File file = new File(path);
            return file.exists() ? 0 : -1;
        }
        return getFileDescriptorFromUri(getDocumentUriFromPath(path), "r") != -1 ? 0 : -1;
    }

    public static int SAF_fopen(String path, String mode) {
        Context context = mSingleton.getApplicationContext();
        Uri uri = null;
        try {
            if (path.startsWith("/data")) {
                // 应用私有目录，使用常规文件 API
                File file = new File(path);
                if (mode.startsWith("w") && !file.exists()) {
                    file.createNewFile();
                    file.setReadable(true);
                    file.setWritable(true);
                }
                uri = Uri.fromFile(file);
            } else if (path.startsWith("/storage")) {
                // 外部存储，使用 SAF
                // 先尝试获取已有文件的 URI
                uri = getDocumentUriFromPath(path);
                if (uri == null && mode.startsWith("w")) {
                    // 文件不存在且需要写入，尝试创建
                    uri = createDocument(path);
                    if (uri != null) {
                        // 创建成功，可选地将其加入缓存（如果 getDocumentUriFromPath 内部有缓存，需要更新）
                        // 因为我们使用的 getDocumentUriFromPath 依赖于目录缓存，添加新文件后，最好使父目录缓存失效，
                        // 或者直接更新缓存。简便起见，可以调用一个方法将新文件的 URI 加入完整路径缓存。
                        // 但为了保持简单，我们可以让后续的 getDocumentUriFromPath 在缓存未命中时重新遍历父目录（因为父目录缓存可能已过时）。
                        // 更好的做法是：在这里手动将新文件的信息添加到目录缓存和完整路径缓存。
                        addToCache(path, uri);
                    }
                }
                if (uri == null) {
                    // 文件不存在且非写模式，或创建失败
                    Log.w(TAG, "File not found or cannot be created: " + path);
                    return -1;
                }
            } else {
                // 不支持的其他路径
                return -1;
            }

            // 调用实际打开文件描述符的函数
            return getFileDescriptorFromUri(uri, mode.substring(0,1));
        } catch (Exception e) {
            Log.w(TAG, "Exception in SAF_fopen: " + e.toString());
            return -1;
        }
    }
    /**
     * 创建新文档
     * 假设 path 是完整路径，如 /storage/emulated/0/sdlpal/newfile.txt
     */
    private static Uri createDocument(String path) throws Exception {
        Context context = mSingleton.getApplicationContext();
        String relativePath = toRelativePath(path);
        if (relativePath == null) return null;

        // 获取父目录路径
        int lastSlash = relativePath.lastIndexOf('/');
        String parentRelativePath = lastSlash > 0 ? relativePath.substring(0, lastSlash) : "";
        String fileName = relativePath.substring(lastSlash + 1);

        // 获取父目录的 DocumentFile
        DocumentFile parentDoc;
        if (parentRelativePath.isEmpty()) {
            // 根目录
            parentDoc = DocumentFile.fromTreeUri(context, docTreeUri);
        } else {
            Uri parentUri = getDocumentUriFromPath(parentRelativePath); // 注意：这里需要根据相对路径或完整路径？
            // 我们需要一个根据相对路径获取父目录 URI 的函数，但 getDocumentUriFromPath 接收完整路径。
            // 因此需要构建父目录的完整路径：basePath + "/" + parentRelativePath
            String parentFullPath = basePath + (parentRelativePath.isEmpty() ? "" : "/" + parentRelativePath);
            parentUri = getDocumentUriFromPath(parentFullPath);
            if (parentUri == null) {
                Log.w(TAG, "Parent directory not found: " + parentFullPath);
                return null;
            }
            parentDoc = DocumentFile.fromSingleUri(context, parentUri);
        }

        if (parentDoc == null || !parentDoc.exists() || !parentDoc.isDirectory()) {
            Log.w(TAG, "Invalid parent directory");
            return null;
        }

        // 确定 MIME 类型（可以根据文件名后缀获取）
        String mimeType = getMimeType(fileName);
        DocumentFile newFile = parentDoc.createFile(mimeType, fileName);
        if (newFile != null) {
            return newFile.getUri();
        }
        return null;
    }

    /**
     * 将文件路径和 URI 添加到缓存（如果使用缓存的话）
     * 这里根据我们之前的单一缓存设计（sSegmentCache）添加条目
     */
    private static void addToCache(String fullPath, Uri uri) {
        String relativePath = toRelativePath(fullPath);
        if (relativePath == null) return;
        // 添加到完整路径缓存（如果有）
        // 根据我们之前的单一缓存设计，可能不需要额外操作，因为我们的 findChildIgnoreCase 内部有目录缓存，
        // 但新文件创建后，父目录的缓存已经过时（因为父目录下多了一个子项）。我们需要使父目录的缓存失效，
        // 或者直接更新父目录的缓存。
        // 简便起见，我们可以清除整个缓存（或者清除父目录相关的缓存条目）。
        // 这里实现一个简单的：清除父目录的缓存条目。
        int lastSlash = relativePath.lastIndexOf('/');
        if (lastSlash > 0) {
            String parentRelative = relativePath.substring(0, lastSlash);
            String parentFullPath = basePath + "/" + parentRelative;
            // 父目录的完整路径，我们需要使父目录在 sSegmentCache 中的相关缓存失效。
            // 但是我们的 sSegmentCache 是分段的，键是 "父目录URI|子项名"。
            // 最简单的方法：清除整个缓存，或者直接删除父目录 URI 对应的所有条目（较复杂）。
            // 为了简化，我们可以不清除缓存，而是信任后续的 exists() 验证会检测到新文件。
            // 但父目录的缓存是子项映射，不会自动包含新文件，所以下次访问新文件时，findChildIgnoreCase 会因为缓存未命中而重新遍历父目录，
            // 这时父目录的遍历会更新缓存，包含新文件。因此，其实不需要显式清除缓存。
            // 但是，如果父目录之前已经被缓存过，那么新文件不会被包含，直到下次访问父目录下的任何子项触发重新遍历。
            // 这可能导致新文件在创建后首次访问时，仍然找不到（因为父目录缓存是旧的），从而重新遍历。
            // 因此，为了确保新文件能立即被找到，我们需要主动使父目录的缓存失效。
            // 下面尝试清除父目录在 sSegmentCache 中相关的所有条目。
            String parentUri = DocumentFile.fromTreeUri(mSingleton.getApplicationContext(), docTreeUri).getUri().toString();
            if (!parentRelative.isEmpty()) {
                // 我们需要逐级获取父目录的 URI，但可以从 getDocumentUriFromPath 获取
                Uri parentDocUri = getDocumentUriFromPath(parentFullPath);
                if (parentDocUri != null) {
                    parentUri = parentDocUri.toString();
                }
            }
            // 清除所有以 parentUri + "|" 开头的缓存键
            String finalParentUri = parentUri;
            sSegmentCache.keySet().removeIf(key -> key.startsWith(finalParentUri + "|"));
        }
    }

    // 辅助方法：根据文件名获取 MIME 类型
    private static String getMimeType(String fileName) {
        String extension = fileName.substring(fileName.lastIndexOf('.') + 1);
        switch (extension.toLowerCase()) {
            case "txt": return "text/plain";
            case "log": return "text/plain";
            case "jpg": case "jpeg": return "image/jpeg";
            case "png": return "image/png";
            // 默认二进制流
            default: return "application/octet-stream";
        }
    }

    protected static void setPersistedUri(Uri uri, boolean save) {
        try{
            if (uri != null) {
                docTreeUri = uri;
                String filePath = getPath(uri);
                setBasePath(filePath);
                if(save)
                    savePersistedUriToCache();
            }
        }catch(Exception e) {
            Log.v(TAG,"setPersistedUri exception:" + e.toString());
        }
    }
    public static void setPersistedUri(Uri uri) {
        setPersistedUri(uri, true);
    }

    public static Uri getDocTreeUri() {
        return docTreeUri;
    }

    protected void loadPersistedUriFromCache() {
        File persistFile = new File(cachePath + "/persisted");
        FileInputStream in;
        try {
            int length = (int) persistFile.length();
            byte[] bytes = new byte[length];
            in = new FileInputStream(persistFile);
            in.read(bytes);
            String contents = new String(bytes);
            Uri uri = Uri.parse(contents);
            setPersistedUri(uri, false); 
            in.close();
            basePath = getPath(uri);
        }catch(FileNotFoundException e) {
            blocked = true;
            alertUser(R.string.toast_requestpermission, new RequestForPermissions() {
                                                            @Override
                                                            public void request() {
                                                                Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
                                                                i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
                                                                mSingleton.permissionRequestLauncher.launch(i);
                                                            }
                                                        });
        }catch(Exception e) {
            Log.w(TAG, "Exception: loadPersistedUriFromCache:"+e.toString());
        }
    }

    protected static void savePersistedUriToCache() {
        if (docTreeUri != null) {
            File persistFile = new File(cachePath + "/persisted");
            FileOutputStream out;
            try {
                out = new FileOutputStream(persistFile);
                out.write(docTreeUri.toString().getBytes());
                out.close();
            } catch(Exception e) {
                Log.w(TAG, "Exception: savePersistedUriToCache:"+e.toString());
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mSingleton = this;
        permissionRequestLauncher = registerForActivityResult(
        new ActivityResultContracts.StartActivityForResult(),
        new ActivityResultCallback<ActivityResult>() {
            @Override
            public void onActivityResult(ActivityResult result) {
                if (result.getResultCode() == Activity.RESULT_OK) {
                    Intent data = result.getData();
                    Uri uri = data.getData();
                    getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                    setPersistedUri(uri);
                    StartGame();
                }
            }
        });
    }

    public void onStart() {
        super.onStart();
        cachePath = getApplicationContext().getCacheDir().getPath();
        loadPersistedUriFromCache();
        String dataPath = getApplicationContext().getFilesDir().getPath();
        String sdlpalPath = basePath;

        if (SettingsActivity.loadConfigFile()) {
            String gamePath =  SettingsActivity.getConfigString(SettingsActivity.GamePath, true);
            if (gamePath != null && !gamePath.isEmpty())
                sdlpalPath = gamePath;
        }
        SetAppPath(sdlpalPath, dataPath, cachePath);

        if (!blocked)
            StartGame();
    }


    public void StartGame() {
        File runningFile = new File(cachePath + "/running");
        crashed = runningFile.exists();

        Intent intent;
        if (SettingsActivity.loadConfigFile() || crashed) {
            runningFile.delete();

            intent = new Intent(this, SettingsActivity.class);
        } else {
            intent = new Intent(this, PalActivity.class);
        }
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(intent);
        finish();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }
}
