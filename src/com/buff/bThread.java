package com.buff;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class bThread extends Activity {
	private Button goButton;
	private TextView notifView;
	private static native void firBuff();
	private buffThread bufferNatStart;
	
	static{
		System.loadLibrary("buffThread");
	}
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        bufferNatStart = new buffThread();
        
        goButton = (Button)this.findViewById(R.id.goButton);
        
        goButton.setOnClickListener(new View.OnClickListener() {
			
			public void onClick(View v) {
				// TODO Auto-generated method stub
				bufferNatStart.start();
			}
		});
    }
    
    @SuppressWarnings("unused")
	private class buffThread extends Thread {
    	public void run()
    	{
    		firBuff();
    	}
    }
}