package com.example.fingerscan;


import java.io.DataOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.sql.Date;
import java.text.SimpleDateFormat;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

public class FingerViewFragment extends Fragment implements OnClickListener {
	static final String TAG = "FingerViewFragment";
	private Bitmap bmp;
	private Bitmap operation;
	public int workingCnt = 0;
	TextView mTextView;
		
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    private ImageView findViewById(int imageview1) {
		// TODO Auto-generated method stub
		return null;
	}

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    	//�߰�
    	Log.d(TAG, "createview ");
    	
		View v = inflater.inflate(R.layout.fingerview, container, false);
		
		Button btn1 = (Button) v.findViewById(R.id.btStartStop);
		btn1.setOnClickListener((OnClickListener) this);
        
        Button btn2 = (Button) v.findViewById(R.id.button2);
        btn2.setOnClickListener((OnClickListener) this);
        
        Button btn3 = (Button) v.findViewById(R.id.button3);
        btn3.setOnClickListener((OnClickListener) this);
        
        Button btn4 = (Button) v.findViewById(R.id.button4);
        btn4.setOnClickListener((OnClickListener) this);
        
        mTextView = (TextView) v.findViewById(R.id.textView1);
    	
        //return super.onCreateView(inflater, container, savedInstanceState);
    	//return inflater.inflate(R.layout.fingerview, container, false);
        return v;
    }
	
	@Override
    public void onClick(View v) {
        switch (v.getId()) {
        case R.id.btStartStop:
        	Log.d(TAG, "onClick button1 ");
        	((MainActivity)getActivity()).FpgaUpgradeFile();
            break;
        case R.id.button2:
        	Log.d(TAG, "onClick button2 ");
        	workingCnt = 0;
        	((MainActivity)getActivity()).FpgaStart();
            break;
        case R.id.button3:
        	Log.d(TAG, "onClick button3 ");
        	((MainActivity)getActivity()).FpgaStop();
            break;
        case R.id.button4:
    	    Log.d(TAG, "onClick button4 ");
    	    screenshot();			
            break;
        }    
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
            View rootView = getActivity().getWindow().getDecorView().getRootView();

            try {
                save = sdCardPath.getPath() + "/" + folder + "/" + dateString;
                // ���� ���
                fos = new FileOutputStream(save);
                rootView.setDrawingCacheEnabled(true);
                Bitmap captureView = rootView.getDrawingCache();
                captureView.compress(Bitmap.CompressFormat.PNG, 100, fos); // ĸ��
                rootView.setDrawingCacheEnabled(false);
                
                // �̵�� ��ĳ�ʸ� ���� ��� �̵�� ����Ʈ�� ���Ž�Ų��.
                getActivity().sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE,                 
                    Uri.parse("file://" + Environment.getExternalStorageDirectory())));
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
            
            Toast.makeText(getActivity().getApplicationContext(), dateString + ".png ����", Toast.LENGTH_LONG).show();
            
          //��������
            dateString = createName(System.currentTimeMillis(), "bin");
            String filename = sdCardPath.getPath() + "/" + folder + "/" + dateString;
            try {
                DataOutputStream out = new DataOutputStream(new FileOutputStream(filename));

                // byte �迭�� �ƴ� ���� �迭�̾��, byte �����Ͱ� ����� �Էµ�
                byte[] b = ((MainActivity)getActivity()).imgbuffer;

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
    }
	
	public void setTextViewText(String value){
        mTextView.setText(value);
    }
	
	public void CreateFingerImageView(byte[] image) {
		// TODO Auto-generated method stub		
		Bitmap tmpBmp;
		int r, g, b, cnt = 0, offset = 1;	
		
		if (image == null)
			image = ((MainActivity)getActivity()).FpgaGetData();
		
		String hexaString = ((MainActivity)getActivity()).byteArrayToHex(image, 10);
        //Log.d(TAG, "CreateFingerImageView hexa = " + hexaString);
        
		ImageView imageView = (ImageView) getView().findViewById(R.id.imageView1);
		
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
	    imageView.setImageBitmap(tmpBmp);
	}
	
	public void checkWorking() {
		workingCnt++;
		if (workingCnt == 10) {
			((MainActivity)getActivity()).FpgaStop();
		}
	}
	
	
}
