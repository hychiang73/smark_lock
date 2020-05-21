package com.example.android.bluetoothlegatt;

import android.util.Log;

public class BackProcess implements Runnable {
    private final static String TAG = BackProcess.class.getSimpleName();
    Thread backgroundThread;

    public void start() {
        if( backgroundThread == null ) {
            backgroundThread = new Thread( this );
            backgroundThread.start();
        }
    }

    public void stop() {
        if( backgroundThread != null ) {
            backgroundThread.interrupt();
        }
    }

    public void run() {
        try {
            Log.w(TAG,"Thread starting.");
            while( !backgroundThread.interrupted() ) {
               // doSomething();
            }
            Log.w(TAG,"Thread stopping.");
        } catch( Exception e ) {
            // important you respond to the InterruptedException and stop processing
            // when its thrown!  Notice this is outside the while loop.
            Log.w(TAG, "Thread shutting down as it was requested to stop.");
        } finally {
            backgroundThread = null;
        }
    }
}
