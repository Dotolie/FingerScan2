package com.example.fingerscan3;
/*
 * AudioVideoPlayerSample
 * Sample project to play audio and video from MPEG4 file using MediaCodec.
 *
 * Copyright (c) 2014 saki t_saki@serenegiant.com
 *
 * File name: PlayerFragment.java
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * All files in the folder are under this Apache License, Version 2.0.
*/

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.IOException;

import com.example.fingerscan3.R;

import android.app.Activity;
import android.app.Fragment;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.ImageView;

@SuppressWarnings("unused")
public class DisplayFragment extends Fragment {
	private static final boolean DEBUG = true;	// TODO set false on release
	private static final String TAG = "DisplayFragment";
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		final View rootView = inflater.inflate(R.layout.displayview, container, false);
		
		Log.d(TAG, "PlayerFragment createview ");
		
		//ImageView imageView = (ImageView) getView().findViewById(R.id.imageView1);

		return rootView;
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
}
