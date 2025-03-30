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

import android.app.Activity;
import android.Manifest;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.*;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.provider.Settings;
import android.net.Uri;
import android.util.Log;

import java.io.*;

public class MainActivity extends Activity {

    static {
        System.loadLibrary("SDL3");
        System.loadLibrary("main");
    }

    private static final String TAG = "sdlpal-debug";

    public static native void setAppPath(String basepath, String datapath, String cachepath);

    public static boolean crashed = false;

    private final static int REQUEST_FILESYSTEM_ACCESS_CODE = 101;

    interface RequestForPermissions {
        void request();
    }

    private void requestForPermissions() {
        // Since granting writing permission implicitly grants reading permission, no need to explicitly add reading permission here
        //ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.MANAGE_EXTERNAL_STORAGE}, REQUEST_FILESYSTEM_ACCESS_CODE);
        try {
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
            intent.addCategory("android.intent.category.DEFAULT");
            intent.setData(Uri.parse(String.format("package:%s",getApplicationContext().getPackageName())));
            startActivityForResult(intent, REQUEST_FILESYSTEM_ACCESS_CODE);
        } catch (Exception e) {
            Intent intent = new Intent();
            intent.setAction(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
            startActivityForResult(intent, REQUEST_FILESYSTEM_ACCESS_CODE);
        }
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        
        if (requestCode == REQUEST_FILESYSTEM_ACCESS_CODE) {
            if (resultCode == Activity.RESULT_OK) {
                StartGame();
            } else {
                alertUser(R.string.toast_requestpermission, new RequestForPermissions() {
                    @Override
                    public void request() {
                        requestForPermissions();
                    }
                });
            }
        }
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

    public void onStart() {
        super.onStart();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M || Environment.isExternalStorageManager() ) {
            StartGame();
        } else {
            alertUser(R.string.toast_requestpermission, new RequestForPermissions() {
                @Override
                public void request() {
                    requestForPermissions();
                }
            });
        }
    }

    private boolean traverseDirectory(String path) {
        File dir = new File(path);
        boolean iteration = true;
        if (dir.exists()) {
            File[] files = dir.listFiles();
            if (files.length == 0) {
                return true;
            } else {
                for (File file: files) {
                    iteration = iteration & (file.isDirectory() ? traverseDirectory(file.getAbsolutePath()) : file.exists());
                }
            }
        }
        return iteration;
    }

    public void StartGame() {
        String dataPath = getApplicationContext().getFilesDir().getPath();
        String cachePath = getApplicationContext().getCacheDir().getPath();
        String sdcardState = Environment.getExternalStorageState();
        String sdlpalPath = Environment.getExternalStorageDirectory().getPath() + "/sdlpal/";

        // hack; on LOS14.1, everytime after rebooted, native file operation will not work until java code accessed same file once, even permission granted.
        traverseDirectory(sdlpalPath);

        String extPath = getApplicationContext().getExternalFilesDir(null).getPath();
		File extFolder = new File(sdlpalPath);
		if( !extFolder.exists() )
			extFolder.mkdirs();
		File forceFile = new File(extPath + "/.force_external_data");
        Log.v(TAG,"checking redirect file path:"+forceFile.getPath());
		if( forceFile.exists() ) {
        	Log.v(TAG,"exist!");
			dataPath = sdlpalPath;
			cachePath = sdlpalPath;
		}else
        	Log.v(TAG,"not exist!");
        if (sdcardState.equals(Environment.MEDIA_MOUNTED)){
            setAppPath(sdlpalPath, dataPath, cachePath);
        } else {
            setAppPath("/sdcard/sdlpal/", dataPath, cachePath);
        }

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
