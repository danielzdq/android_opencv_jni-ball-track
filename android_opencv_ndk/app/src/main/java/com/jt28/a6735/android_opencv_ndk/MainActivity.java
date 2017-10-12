package com.jt28.a6735.android_opencv_ndk;

import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

import com.jt28.a6735.ble.BleController;
import com.jt28.a6735.ble.callback.ConnectCallback;
import com.jt28.a6735.ble.callback.OnReceiverCallback;
import com.jt28.a6735.ble.callback.OnWriteCallback;
import com.jt28.a6735.ble.callback.ScanCallback;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback,Camera.PreviewCallback{
    private Bitmap bitmap;
    private Bitmap bitmap_logo;
    private Button button;
    private ImageView imageView,imageView_yang;
    //蓝牙发送队列
    private Queue<byte[]> queue = new LinkedList<>();
    //蓝牙
    private BleController mBleController;
    //搜索结果列表
    private List<BluetoothDevice> bluetoothDevices = new ArrayList<BluetoothDevice>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        button = (Button)findViewById(R.id.button);
        bitmap  = BitmapFactory.decodeResource(getResources(),R.drawable.pptimg);
        bitmap_logo  = BitmapFactory.decodeResource(getResources(),R.drawable.dota_logo);
        imageView= (ImageView)findViewById(R.id.img);
        imageView_yang = (ImageView)findViewById(R.id.img_yang);
        imageView_yang.setImageBitmap(bitmap);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        int w = bitmap.getWidth(), h = bitmap.getHeight();
                        int[] pix = new int[w * h];
                        bitmap.getPixels(pix, 0, w, 0, 0, w, h);

                        int l_w = bitmap_logo.getWidth(), l_h = bitmap_logo.getHeight();
                        int[] l_pix = new int[l_w * l_h];
                        bitmap_logo.getPixels(l_pix, 0, l_w, 0, 0, l_w, l_h);
                        //int [] resultPixes=OpenCVHelper.roi_add(pix,l_pix,w,h,l_w,l_h);
                        int [] resultPixes=OpenCVHelper.gray(pix,w,h);//调用native方法
                        Bitmap result = Bitmap.createBitmap(w,h, Bitmap.Config.RGB_565);
                        result.setPixels(resultPixes, 0, w, 0, 0,w, h);
                        imageView.setImageBitmap(result);
                    }
                }).run();

            }
        });
        SurfaceView view = (SurfaceView) findViewById(R.id.surface_view);
        view.getHolder().addCallback(this);
        view.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

        //添加元素
        byte[] a = new byte[5];
        a[0] = (byte)0xff;
        queue.offer(a);
        queue.poll(); //返回第一个元素，并在队列中删除

//        // TODO  第一步：初始化
//        mBleController = BleController.getInstance().init(this);
//        // TODO  第二步：搜索设备，获取列表后进行展示
//        scanDevices();
    }

    private void scanDevices() {
        mBleController.scanBle(0, new ScanCallback() {
            @Override
            public void onSuccess() {
                if (bluetoothDevices.size() > 0) {
//                    mDeviceList.setAdapter(new DeviceListAdapter(MainActivity.this, bluetoothDevices));
//                    mDeviceList.setOnItemClickListener(MainActivity.this);
                } else {
                    Toast.makeText(MainActivity.this, "未搜索到Ble设备", Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onScanning(BluetoothDevice device, int rssi, byte[] scanRecord) {
                if (!bluetoothDevices.contains(device)) {
                    bluetoothDevices.add(device);
                }
            }
        });

        // TODO 第三步：点击条目后,获取地址，根据地址连接设备
        String address = bluetoothDevices.get(0).getAddress();
        mBleController.connect(0, address, new ConnectCallback() {
            @Override
            public void onConnSuccess() {
                Toast.makeText(MainActivity.this, "连接成功", Toast.LENGTH_SHORT).show();
                // TODO 在新的界面要获取实例，无需init
                //mBleController = BleController.getInstance();
                // TODO 接收数据的监听
                mBleController.registReciveListener("MainActivity", new OnReceiverCallback() {
                    @Override
                    public void onRecive(byte[] value) {

                    }
                });
            }

            @Override
            public void onConnFailed() {
                Toast.makeText(MainActivity.this, "连接超时，请重试", Toast.LENGTH_SHORT).show();
            }
        });

        byte[] bytes = new byte[7];//= sendText.getBytes();
        mBleController.writeBuffer(bytes, new OnWriteCallback() {
            @Override
            public void onSuccess() {
                Toast.makeText(MainActivity.this, "发送成功！", Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onFailed(int state) {
                Toast.makeText(MainActivity.this, "发送失败！", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private SurfaceHolder surfaceHolder ;
    private Camera camera ;
    @Override
    public void onPreviewFrame(byte[] bytes, Camera camera) {
        Camera.Parameters parameters = camera.getParameters();
        int imageFormat = parameters.getPreviewFormat();
        int w = parameters.getPreviewSize().width;
        int h = parameters.getPreviewSize().height;
        Rect rect = new Rect(0, 0, w, h);
        YuvImage yuvImg = new YuvImage(bytes, imageFormat, w, h, null);
        try {
            ByteArrayOutputStream outputstream = new ByteArrayOutputStream();
            yuvImg.compressToJpeg(rect, 100, outputstream);
            Bitmap rawbitmap = BitmapFactory.decodeByteArray(outputstream.toByteArray(), 0, outputstream.size());

            int wx = rawbitmap.getWidth(), hx = rawbitmap.getHeight();
            int[] pix = new int[w * h];
            rawbitmap.getPixels(pix, 0, wx, 0, 0, wx, hx);
            int [] resultPixes=OpenCVHelper.gray(pix,wx,hx);//调用native方法
            Bitmap result = Bitmap.createBitmap(wx,hx, Bitmap.Config.RGB_565);
            result.setPixels(resultPixes, 0, wx, 0, 0,wx, hx);
            imageView.setImageBitmap(result);
        } catch (Exception e) {
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        try{
            camera = Camera.open(1);
            camera.setPreviewDisplay(surfaceHolder);
            Camera.Parameters params = camera.getParameters();
            params.setPreviewSize(352, 288);
            camera.setParameters(params);
            camera.startPreview() ;
            camera.setPreviewCallback(this);
            setCameraDisplayOrientation(this, 1, camera);
        }catch(Exception e){
            e.printStackTrace();
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        if(camera != null) camera.release() ;
        camera = null ;
    }


    /**
     * 旋转屏幕后自动适配（若只用到竖的，也可不要）
     * 已经在manifests中让此Activity只能竖屏了
     * @param activity 相机显示在的Activity
     * @param cameraId 相机的ID
     * @param camera 相机对象
     */
    private static void setCameraDisplayOrientation(Activity activity, int cameraId, Camera camera)
    {
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(cameraId, info);
        int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (rotation)
        {
            case Surface.ROTATION_0: degrees = 0; break;
            case Surface.ROTATION_90: degrees = 90; break;
            case Surface.ROTATION_180: degrees = 180; break;
            case Surface.ROTATION_270: degrees = 270; break;
        }
        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT)
        {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360;  // compensate the mirror
        }
        else
        {
            // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        camera.setDisplayOrientation(result);
    }
}
