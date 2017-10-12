package com.jt28.a6735.android_opencv_ndk;

public class OpenCVHelper {
    static {
        System.loadLibrary("OpenCV");
    }
    public static native int[] gray(int[] buf, int w, int h);
    public static native int[] roi_add(int[] buf,int[] buf2,int w, int h,int l_w, int l_h);
}
