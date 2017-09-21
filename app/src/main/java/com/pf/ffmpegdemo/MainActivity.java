package com.pf.ffmpegdemo;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    public void load(View view) {
        File inputFile = new File(Environment.getExternalStorageDirectory(), "input.mp4");
        if (!inputFile.exists()) {
            Toast.makeText(this, "文件不存在", Toast.LENGTH_SHORT).show();
            return;
        }
        String inputStr = inputFile.getAbsolutePath();
        File outputFile = new File(Environment.getExternalStorageDirectory(), "output.yuv");
        String outStr = outputFile.getAbsolutePath();
        if (outputFile.exists()) {
            outputFile.delete();
        }
        openFile(inputStr, outStr);
    }

    public native void openFile(String inputFileName, String outFileName);
}