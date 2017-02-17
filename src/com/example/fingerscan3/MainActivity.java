package com.example.fingerscan3;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.sql.Date;
import java.text.SimpleDateFormat;

import android.app.Activity;
import android.app.Fragment;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.VideoView;

public class MainActivity extends Activity {
	public static final String TAG = "FingerScan";
	private final static String FRAGMENT_TAG = "FRAGMENTB_TAG";
	private static final int MESSAGE_READ = 0;
	private static final int MESSAGE_START_CAPTURE = 1;
	private static final int MESSAGE_STOP_CAPTURE = 2;
	private static final int MESSAGE_START_SCAN = 3;
	private static final int MESSAGE_STOP_SCAN = 4;
	
	final static String SAMPLE_VIDEO_URL = "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4";
	private Fragment fr1;
	private Fragment fr2;

	public Button mBtStartStop;
	public Button mBtCapture;
	public ImageView mIvFinger;
	public ProgressBar mPbProgress;
	public int mCount = 0;
	public boolean mRun = false;
	
    VideoView videoView;
    byte[] imgbuffer= new byte[200 * 200];
    Handler updateHandler = new Handler();
    
    public TextView mTvTime;
    public long startTime;
    public long elapsedTime;
    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);		
		setContentView(R.layout.activity_main);
		setViews();
		CallBackInit();

	}
	
	/** Called when the activity is about to be destroyed. */
    @Override
    protected void onDestroy()
    {
    	fpgaStop();
    	DeviceClose();
        super.onDestroy();
    }

	// byte[] to hex
	public static String byteArrayToHex(byte[] ba, int count) {
	    if (ba == null || ba.length == 0) {
	        return null;
	    }

	    StringBuffer sb = new StringBuffer(ba.length * 2);
	    String hexNumber;
	    for (int x = 0; x < count; x++) {
	        hexNumber = "0" + Integer.toHexString(0xff & ba[x]);
	        sb.append(hexNumber.substring(hexNumber.length() - 2));
	    }
	    return sb.toString();
	}
	
	private final Handler handler = new Handler() 
	{
		@Override
		public void handleMessage(Message msg) {
	        switch (msg.what) {
	          case MESSAGE_READ:
	        	  int size;
	        	byte[] readBuf = (byte[]) msg.obj;
	        	size = 200 * 200 * 2;//20161228 kimjs
	            if (readBuf == null)
	            	return;
	            
	            imgbuffer = new byte[size];
	            System.arraycopy(readBuf, 0, imgbuffer, 0, size);
//	     	    FingerViewFragment fingerFmt = (FingerViewFragment) getFragmentManager().findFragmentById(R.id.fingerview_fragment);
//	     	    DisplayFragment displayFmt = (DisplayFragment) getFragmentManager().findFragmentById(R.id.displayview_fragment);
	     	    //if (fingerFmt != null)
	            //	fingerFmt.CreateFingerImageView(readBuf);
	     	    //	fingerFmt.checkWorking();
//	     	    if (displayFmt != null)
//	     	    	displayFmt.CreateFingerImageView(readBuf);
	            
	            runOnUiThread(new Runnable() {
	            	@Override
	            	public void run() {
	            		mCount++;
	            		mIvFinger.setImageBitmap(CreateFingerImageView(imgbuffer));
	            		mPbProgress.setProgress(mCount*100/25);
	            		elapsedTime  = (System.currentTimeMillis() - startTime)/1000;
	            		mTvTime.setText(""+elapsedTime+" sec");
	            		if( mCount == 23 ) {
	            			mTvTime.setTextColor(0xffffffff);
	            		}
	            		if( mCount >= 25) {
	            			FpgaStop();
							mBtStartStop.setText("Start");
							mBtCapture.setEnabled(true);
							mCount = 0;
							mRun = false;
	            		}
	            	}
	            });
	     	    break;
			case MESSAGE_STOP_CAPTURE:
				mBtStartStop.setEnabled(true);
				break;
			case MESSAGE_START_CAPTURE:
				mBtStartStop.setEnabled(false);
				capture();
				break;
			case MESSAGE_START_SCAN:
				mCount = 0;
				mRun = true;
				FpgaStart();
				mBtStartStop.setText("Stop");
				mBtCapture.setEnabled(false);
				mTvTime.setTextColor(0xff157efb);
	            startTime = System.currentTimeMillis();
				break;
			case MESSAGE_STOP_SCAN:
				mRun = false;
				FpgaStop();
				mBtStartStop.setText("Start");
				mBtCapture.setEnabled(true);
				break;
			}
	    }

		private Activity getActivity() {
			// TODO Auto-generated method stub
			return null;
		}
    };	

    public void setViews() {
    	mBtStartStop = (Button)findViewById(R.id.btStartStop);
    	mBtStartStop.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				if( mRun ) {
					handler.obtainMessage(MESSAGE_STOP_SCAN).sendToTarget();
				}
				else {
					handler.obtainMessage(MESSAGE_START_SCAN).sendToTarget();
				}
			}
		});
    	mBtCapture = (Button)findViewById(R.id.btCapture);
    	mBtCapture.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				handler.obtainMessage(MESSAGE_START_CAPTURE).sendToTarget();	
			}
		});
    	mIvFinger = (ImageView)findViewById(R.id.ivFinger);
    	mPbProgress = (ProgressBar)findViewById(R.id.pbProgress);
    	mTvTime = (TextView)findViewById(R.id.tvTime);
    }
    
	public Bitmap CreateFingerImageView(byte[] image) {
		// TODO Auto-generated method stub		
		Bitmap tmpBmp;
		int r, g, b, cnt = 0, offset = 1;	
		
		if (image == null)
			image = FpgaGetData();
		
//		String hexaString = ((MainActivity)getActivity()).byteArrayToHex(image, 10);
        //Log.d(TAG, "CreateFingerImageView hexa = " + hexaString);
        
//		ImageView imageView = (ImageView) getView().findViewById(R.id.imageView1);
		
		tmpBmp = Bitmap.createBitmap(200, 200, Bitmap.Config.ARGB_8888);
	    
	    for(int x = 0; x < 200; x++) {
	      	for(int y = 0; y < 200; y++) {
	       		r = (int)(0xFF & image[(cnt * 2) + offset]);
	            g = (int)(0xFF & image[(cnt * 2) + offset]);
	            b = (int)(0xFF & image[(cnt * 2) + offset]);
	            tmpBmp.setPixel(x, y, Color.rgb(r, g, b));
	            cnt++;
	       	}
	    }
	    return tmpBmp;
	}   
	
	private void capture() {
		Thread mth = new Thread( new Runnable() {
			@Override
			public void run() {
				screenshot();
			}
		});
		mth.start();
	}
	private String createName(long dateTaken, String ext){
		Log.d("FingerScan", "createName");
        Date date = new Date(dateTaken);
        SimpleDateFormat dateFormat = 
        new SimpleDateFormat("yyyy-MM-dd HH.mm.ss");
        return dateFormat.format(date)+"." + ext;
    }
	
	public void screenshot() {
		FileOutputStream fos;
        String save;
		String folder = "FingerScan"; // ���� �̸�
		Log.d("FingerScan", "screenshot");
		
        try {
            // ���� ��¥�� ������ �����ϱ�
            String dateString = createName(System.currentTimeMillis(), "png");
            File sdCardPath = Environment.getExternalStorageDirectory();
            File dirs = new File(Environment.getExternalStorageDirectory(), folder); 
            
            if (!dirs.exists()) { // ���ϴ� ��ο� ������ �ִ��� Ȯ��
                dirs.mkdirs(); // Test ���� ����
                Log.d("FingerScan", "Directory Created");
            }            
            
            //View rootView = findViewById(android.R.id.content).getRootView();		//no fragment
            View rootView = getWindow().getDecorView().getRootView();

            try {
                save = sdCardPath.getPath() + "/" + folder + "/" + dateString;
                // ���� ���
                fos = new FileOutputStream(save);
                rootView.setDrawingCacheEnabled(true);
                Bitmap captureView = rootView.getDrawingCache();
                captureView.compress(Bitmap.CompressFormat.PNG, 100, fos); // ĸ��
                rootView.setDrawingCacheEnabled(false);
                
                // �̵�� ��ĳ�ʸ� ���� ��� �̵�� ����Ʈ�� ���Ž�Ų��.
                sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE,                 
                    Uri.parse("file://" + Environment.getExternalStorageDirectory())));
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
            
//            Toast.makeText(getApplicationContext(), dateString + ".png", Toast.LENGTH_LONG).show();
            
          //��������
            dateString = createName(System.currentTimeMillis(), "bin");
            String filename = sdCardPath.getPath() + "/" + folder + "/" + dateString;
            try {
                DataOutputStream out = new DataOutputStream(new FileOutputStream(filename));

                // byte �迭�� �ƴ� ���� �迭�̾��, byte �����Ͱ� ����� �Էµ�
                byte[] b = imgbuffer;

                for (int i = 0; i < b.length; i++)
                   out.write(b[i]);

                out.close();
                Log.d("FingerScan", "file save end");
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
        } catch (Exception e) {
            // TODO: handle exception
            Log.e("FingerScan", "" + e.toString());
        }
        
        handler.obtainMessage(MESSAGE_STOP_CAPTURE).sendToTarget();
    }
	
    public void Callback(byte[] pByte) {
    	int bytes;

		bytes  = 200 * 200 * 2;

		handler.obtainMessage(MESSAGE_READ, bytes, -1, pByte)
		             .sendToTarget();
    }
	
    public void FpgaUpgradeFile() {
    	flashUpdate("/mnt/sdcard/1212_rev.bin");
    }
    
    public void FpgaStart() {
    	fpgaStart(1);
    }
    
    public void FpgaStop() {
    	fpgaStop();
    }
    
    public byte[] FpgaGetData() {
    	return GetData();
    }
    
    //fpga native api
    public native int fpgaStart(int freq);
    public native int fpgaStop();
    public native int flashRead(String filename);
    public native int flashUpdate(String filename);
    
    public native byte[] GetData();
    
    public native void CallBackInit();
    
    public native void DeviceInit();
    public native void DeviceClose();
    
    //gpio native api
    public native void GpioSetDirection(String port, boolean dir);
    public native void GpioSetValue(String port, boolean value);
    public native boolean GpioGetValue(String port);

    static {
        System.loadLibrary("fingerscan");
    }
}
