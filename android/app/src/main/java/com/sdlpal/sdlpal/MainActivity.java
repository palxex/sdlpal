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

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("SDL3");
        System.loadLibrary("main");
    }

    private static MainActivity mSingleton;
    private ActivityResultLauncher<Intent> permissionRequestLauncher;

    private static final String TAG = "sdlpal-debug";

    private static Uri docTreeUri = null;
    private static String docUri = null;
    private static String basePath = "";
    private static String dataPath = "";
    private static String cachePath = "";

    public static String getBasePath() {
        return basePath;
    }

    public static void setBasePath(String basepath) {
        basePath = basepath;
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
            Context ctx = mSingleton.getApplicationContext();
            path = path.replace(basePath, "/");
            path = path.replace("/","%2F");
            path = path.replace("%2F%2F","%2F");
            path = docUri + path;
            String documentId = DocumentsContract.getDocumentId(Uri.parse(path));
            Uri documentUri = DocumentsContract.buildDocumentUriUsingTree(docTreeUri, documentId);
            return documentUri;
        } catch (Exception e) {
            Log.w(TAG, "Exception: getDocumentUriFromPath:" + e.toString());
        }
        return null;
    }

    public static int SAF_access(String path, int mode) {
        if( path.startsWith("/data/") ) {
            File file = new File(path);
            return file.exists() ? 0 : -1;
        }
        return getFileDescriptorFromUri(getDocumentUriFromPath(path), "r") != -1 ? 0 : -1;
    }

    public static int SAF_fopen(String path, String mode) {
        Uri uri = getDocumentUriFromPath(path);
        mode = mode.substring(0,1);
        try {
            File file = new File(path);
            if( mode.equals("w") && !file.exists() ){
                if( path.startsWith("/data") ) {
                    file.createNewFile();
                    file.setReadable(true);
                    file.setWritable(true);
                }else if( path.startsWith("/storage") ) {
                    DocumentFile documentFile = DocumentFile.fromTreeUri(mSingleton.getApplicationContext(), docTreeUri);
                    if (documentFile != null) {
                        DocumentFile newFile = documentFile.createFile("application/octet-stream", file.getName());
                        if (newFile != null) {
                            uri = newFile.getUri();
                        } 
                    }
                }
            }
            if(path.startsWith("/data"))
                uri = Uri.fromFile( file );
        }catch (Exception e) {
            Log.w(TAG, "Exception: SAF_fopen create file for writing:" + e.toString());
        }
        return getFileDescriptorFromUri(uri, mode);
    }

    protected static void setPersistedUri(Uri uri, boolean save) {
        if (uri != null) {
            docTreeUri = uri;
            docUri = uri.toString().replace("tree", "document");
            savePersistedUriToCache();
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
            setPersistedUri(Uri.parse(contents), false); 
            in.close();
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
        mSingleton = this;
        super.onCreate(savedInstanceState);
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
        String dataPath = getApplicationContext().getFilesDir().getPath();
        String cachePath = getApplicationContext().getCacheDir().getPath();
        String sdcardState = Environment.getExternalStorageState();
        String sdlpalPath = Environment.getExternalStorageDirectory().getPath() + "/sdlpal/";

        String extPath = getApplicationContext().getExternalFilesDir(null).getPath();
		File extFolder = new File(sdlpalPath);
		if( !extFolder.exists() )
			extFolder.mkdirs();
        if (sdcardState.equals(Environment.MEDIA_MOUNTED)){
            SetAppPath(sdlpalPath, dataPath, cachePath);
        } else {
            SetAppPath("/sdcard/sdlpal/", dataPath, cachePath);
        }
        loadPersistedUriFromCache();

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
