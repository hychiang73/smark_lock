/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.android.bluetoothlegatt;

import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.provider.Settings;
import android.text.Html;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.ExpandableListView;
import android.widget.SimpleExpandableListAdapter;
import android.widget.TextView;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;

/**
 * For a given BLE device, this Activity provides the user interface to connect, display data,
 * and display GATT services and characteristics supported by the device.  The Activity
 * communicates with {@code BluetoothLeService}, which in turn interacts with the
 * Bluetooth LE API.
 */
public class DeviceControlActivity extends Activity {
    private final static String TAG = DeviceControlActivity.class.getSimpleName();
    private final DateFormat mTimeFormat = DateFormat.getTimeInstance();

    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

    private TextView mConnectionState;
    private TextView mDataField;
    private String mDeviceName;
    private String mDeviceAddress;
    private ExpandableListView mGattServicesList;
    private BluetoothLeService mBluetoothLeService;
    private BackProcess mBackProcess;
    private ArrayList<ArrayList<BluetoothGattCharacteristic>> mGattCharacteristics =
            new ArrayList<ArrayList<BluetoothGattCharacteristic>>();
    private boolean mConnected = false;
    private BluetoothGattCharacteristic mNotifyCharacteristic;

    private final String LIST_NAME = "NAME";
    private final String LIST_UUID = "UUID";

    private BluetoothGatt mBluetoothGatt;

    private EditText code_text;
    public static TextView lock_view;
    private TextView phone_id_view;
    private String phone_id;
    //TextView result;
    private static DeviceControlActivity ins;

    public static int SMARTLOCKER_UNUSABLE = 2;
    public static int SMARTLOCKER_LOCKED = 1;
    public static int SMARTLOCKER_UNLOCKED = 0;
    public static int lock_status = SMARTLOCKER_LOCKED;

    // Code to manage Service lifecycle.
    private final ServiceConnection mServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName componentName, IBinder service) {
            mBluetoothLeService = ((BluetoothLeService.LocalBinder) service).getService();
            if (!mBluetoothLeService.initialize()) {
                Log.e(TAG, "Unable to initialize Bluetooth");
                finish();
            }
            // Automatically connects to the device upon successful start-up initialization.
            mBluetoothLeService.connect(mDeviceAddress);
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            mBluetoothLeService.disconnect();
            mBluetoothLeService = null;
        }
    };

    // Handles various events fired by the Service.
    // ACTION_GATT_CONNECTED: connected to a GATT server.
    // ACTION_GATT_DISCONNECTED: disconnected from a GATT server.
    // ACTION_GATT_SERVICES_DISCOVERED: discovered GATT services.
    // ACTION_DATA_AVAILABLE: received data from the device.  This can be a result of read
    //                        or notification operations.
    private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BluetoothLeService.ACTION_GATT_CONNECTED.equals(action)) {
                Log.w(TAG, "connected to a GATT server.");
                mConnected = true;
                updateConnectionState(R.string.connected);
                invalidateOptionsMenu();
            } else if (BluetoothLeService.ACTION_GATT_DISCONNECTED.equals(action)) {
                Log.w(TAG, "disconnected from a GATT server");
                mConnected = false;
                updateConnectionState(R.string.disconnected);
                invalidateOptionsMenu();
                clearUI();
            } else if (BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED.equals(action)) {
                Log.w(TAG, "discovered GATT services");
                // Show all the supported services and characteristics on the user interface.
                //displayGattServices(mBluetoothLeService.getSupportedGattServices());
                mBluetoothLeService.SetCharNotification();
            } else if (BluetoothLeService.ACTION_DATA_AVAILABLE.equals(action)) {
                Log.w(TAG, "received data from the device.");
                displayData(intent.getStringExtra(BluetoothLeService.EXTRA_DATA));
            }
        }
    };

    // If a given GATT characteristic is selected, check for supported features.  This sample
    // demonstrates 'Read' and 'Notify' features.  See
    // http://d.android.com/reference/android/bluetooth/BluetoothGatt.html for the complete
    // list of supported characteristic features.
    private final ExpandableListView.OnChildClickListener servicesListClickListner =
            new ExpandableListView.OnChildClickListener() {
                @Override
                public boolean onChildClick(ExpandableListView parent, View v, int groupPosition,
                                            int childPosition, long id) {
                    if (mGattCharacteristics != null) {
                        final BluetoothGattCharacteristic characteristic =
                                mGattCharacteristics.get(groupPosition).get(childPosition);
                        final int charaProp = characteristic.getProperties();
                        if ((charaProp | BluetoothGattCharacteristic.PROPERTY_READ) > 0) {
                            // If there is an active notification on a characteristic, clear
                            // it first so it doesn't update the data field on the user interface.
                            if (mNotifyCharacteristic != null) {
                                mBluetoothLeService.setCharacteristicNotification(
                                        mNotifyCharacteristic, false);
                                mNotifyCharacteristic = null;
                            }
                            mBluetoothLeService.readCharacteristic(characteristic);
                        }
                        if ((charaProp | BluetoothGattCharacteristic.PROPERTY_NOTIFY) > 0) {
                            mNotifyCharacteristic = characteristic;
                            mBluetoothLeService.setCharacteristicNotification(
                                    characteristic, true);
                        }
                        return true;
                    }
                    return false;
                }
    };

    private void clearUI() {
        //mGattServicesList.setAdapter((SimpleExpandableListAdapter) null);
        mDataField.setText(R.string.no_data);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.gatt_services_characteristics);
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN|
                WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        setContentView(R.layout.button_control);

        final Intent intent = getIntent();
        mDeviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
        mDeviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);

        // Sets up UI references.
        /*
        ((TextView) findViewById(R.id.device_address)).setText(mDeviceAddress);
        mGattServicesList = (ExpandableListView) findViewById(R.id.gatt_services_list);
        mGattServicesList.setOnChildClickListener(servicesListClickListner);
        mConnectionState = (TextView) findViewById(R.id.connection_state);
         */
        //mDataField = (TextView) findViewById(R.id.data_value);

        lock_view = (TextView) findViewById(R.id.lock_view);
        UpdateLockStatus(0xD0);

        phone_id_view = (TextView) findViewById(R.id.phone_id);
        phone_id = Settings.Secure.getString(getContentResolver(), Settings.Secure.ANDROID_ID);
        phone_id_view.setText("Phone ID: "+ phone_id);

        code_text = (EditText) findViewById(R.id.code_text);
        ins = this;

        getActionBar().setTitle(mDeviceName);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        Intent gattServiceIntent = new Intent(this, BluetoothLeService.class);
        bindService(gattServiceIntent, mServiceConnection, BIND_AUTO_CREATE);

        //SendAppReady();
        //mBackProcess.start();
    }

    @Override
    protected void onResume() {
        super.onResume();
        registerReceiver(mGattUpdateReceiver, makeGattUpdateIntentFilter());
        if (mBluetoothLeService != null) {
            final boolean result = mBluetoothLeService.connect(mDeviceAddress);
            Log.d(TAG, "Connect request result=" + result);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mGattUpdateReceiver);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unbindService(mServiceConnection);
        mBluetoothLeService = null;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.gatt_services, menu);
        if (mConnected) {
            menu.findItem(R.id.menu_connect).setVisible(false);
            menu.findItem(R.id.menu_disconnect).setVisible(true);
        } else {
            menu.findItem(R.id.menu_connect).setVisible(true);
            menu.findItem(R.id.menu_disconnect).setVisible(false);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
            case R.id.menu_connect:
                mBluetoothLeService.connect(mDeviceAddress);
                return true;
            case R.id.menu_disconnect:
                mBluetoothLeService.disconnect();
                return true;
            case android.R.id.home:
                onBackPressed();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void updateConnectionState(final int resourceId) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
               // mConnectionState.setText(resourceId);
            }
        });
    }

    private void displayData(String data) {
        if (data != null) {
            mDataField.setText(data);
        }
    }

    // Demonstrates how to iterate through the supported GATT Services/Characteristics.
    // In this sample, we populate the data structure that is bound to the ExpandableListView
    // on the UI.
    private void displayGattServices(List<BluetoothGattService> gattServices) {
        if (gattServices == null) return;
        String uuid = null;
        String unknownServiceString = getResources().getString(R.string.unknown_service);
        String unknownCharaString = getResources().getString(R.string.unknown_characteristic);
        ArrayList<HashMap<String, String>> gattServiceData = new ArrayList<HashMap<String, String>>();
        ArrayList<ArrayList<HashMap<String, String>>> gattCharacteristicData
                = new ArrayList<ArrayList<HashMap<String, String>>>();
        mGattCharacteristics = new ArrayList<ArrayList<BluetoothGattCharacteristic>>();

        // Loops through available GATT Services.
        for (BluetoothGattService gattService : gattServices) {
            HashMap<String, String> currentServiceData = new HashMap<String, String>();
            uuid = gattService.getUuid().toString();
            currentServiceData.put(
                    LIST_NAME, SampleGattAttributes.lookup(uuid, unknownServiceString));
            currentServiceData.put(LIST_UUID, uuid);
            gattServiceData.add(currentServiceData);

            ArrayList<HashMap<String, String>> gattCharacteristicGroupData =
                    new ArrayList<HashMap<String, String>>();
            List<BluetoothGattCharacteristic> gattCharacteristics =
                    gattService.getCharacteristics();
            ArrayList<BluetoothGattCharacteristic> charas =
                    new ArrayList<BluetoothGattCharacteristic>();

            // Loops through available Characteristics.
            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                charas.add(gattCharacteristic);
                HashMap<String, String> currentCharaData = new HashMap<String, String>();
                uuid = gattCharacteristic.getUuid().toString();
                currentCharaData.put(
                        LIST_NAME, SampleGattAttributes.lookup(uuid, unknownCharaString));
                currentCharaData.put(LIST_UUID, uuid);
                gattCharacteristicGroupData.add(currentCharaData);
                Log.w(TAG, "UUID :" + uuid);
            }
            mGattCharacteristics.add(charas);
            gattCharacteristicData.add(gattCharacteristicGroupData);
        }

        SimpleExpandableListAdapter gattServiceAdapter = new SimpleExpandableListAdapter(
                this,
                gattServiceData,
                android.R.layout.simple_expandable_list_item_2,
                new String[] {LIST_NAME, LIST_UUID},
                new int[] { android.R.id.text1, android.R.id.text2 },
                gattCharacteristicData,
                android.R.layout.simple_expandable_list_item_2,
                new String[] {LIST_NAME, LIST_UUID},
                new int[] { android.R.id.text1, android.R.id.text2 }
        );
        mGattServicesList.setAdapter(gattServiceAdapter);
    }

    private static IntentFilter makeGattUpdateIntentFilter() {
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_CONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_DISCONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(BluetoothLeService.ACTION_DATA_AVAILABLE);
        return intentFilter;
    }

    public void onClickUpdateCode(View v) {
        int size = 3;
        byte[] value = {0};
        String AlphaNumericString = "0123456789";
        StringBuilder sb = new StringBuilder(size);
        String tmp = "Generate new access code ";

        if (mBluetoothLeService == null) {
            Log.w(TAG, "bt service is null");
            return;
        }

        Date now = new Date();
        long timestamp = now.getTime();
        SimpleDateFormat sdf = new SimpleDateFormat("yyMMddHHmm");
        String dateStr = sdf.format(timestamp);

        sb.append(mBluetoothLeService.SL_UPDATE_CODE_CMD + "3");

        for (int i = 0; i < size; i++) {
            int index = (int)(AlphaNumericString.length() * Math.random());
            char c = AlphaNumericString.charAt(index);
            sb.append(c);
            tmp = tmp + Character.toString(c);
            tmp += ", ";
        }

        sb.append(phone_id);
        sb.append(dateStr);

        Log.w(TAG, "Update code = " + sb.toString());

        value = sb.toString().getBytes();

        updateTheTextView(" <font color=\\\"#FE6026\\\">" + tmp + "</font>");

        mBluetoothLeService.writeCustomCharacteristic(value);
    }

    public void SendAppReady() {
        byte[] value = {0};
        String s = "";

        s = Integer.toString(mBluetoothLeService.SL_APP_READY_CMD);
        value = s.getBytes();
        Log.w(TAG, "Send Ready CMD to Smartlock");
        mBluetoothLeService.writeCustomCharacteristic(value);
    }

    public void onClickWrite(View v){
        byte[] value = {0};
        String code = code_text.getText().toString();
        String s = "";

        if (mBluetoothLeService == null) {
            Log.w(TAG, "code or bt service is null");
            return;
        }

        if (code.isEmpty()) {
            showAlert("ERROR", "Please input a valid code");
            return;
        }

        if (lock_status != SMARTLOCKER_UNLOCKED) {
            Date now = new Date();
            long timestamp = now.getTime();
            SimpleDateFormat sdf = new SimpleDateFormat("yyMMddHHmm");
            String dateStr = sdf.format(timestamp);

            s = Integer.toString(mBluetoothLeService.SL_UNLOCK_CMD) + code.length() + code + phone_id + dateStr;
            Log.w(TAG, "unlock code = " + s);

            value = s.getBytes();
            updateTheTextView(" <font color=\\\"#FE6026\\\">" + "Sending access code " + code + " to unlock Locker" + "</font>");
            mBluetoothLeService.writeCustomCharacteristic(value);
        } else {
            updateTheTextView(" <font color=\\\"#FE6026\\\">" + "Locker is already unlocked" + "</font>");
        }

        code_text.getText().clear();

        InputMethodManager inputManager = ( InputMethodManager ) getSystemService(this.INPUT_METHOD_SERVICE);
        inputManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
    }

    public void OnClickSendLock(View v) {
        byte[] value = {0};
        String code = code_text.getText().toString();
        String s = "";

        if (mBluetoothLeService == null) {
            Log.w(TAG, "code or bt service is null");
            return;
        }

        if (lock_status != SMARTLOCKER_LOCKED) {
            Date now = new Date();
            long timestamp = now.getTime();
            SimpleDateFormat sdf = new SimpleDateFormat("yyMMddHHmm");
            String dateStr = sdf.format(timestamp);

            s = Integer.toString(mBluetoothLeService.SL_LOCK_CMD) + code.length() + code + phone_id + dateStr;
            Log.w(TAG, "unlock code = " + s);

            value = s.getBytes();

            updateTheTextView(" <font color=\\\"#FE6026\\\">" + "Trying to lock Locker ..." + "</font>");

            mBluetoothLeService.writeCustomCharacteristic(value);
        } else {
            updateTheTextView(" <font color=\\\"#FE6026\\\">" + "Locker is already locked" + "</font>");
        }
    }

    public void OnClickReset(View v) {
        byte[] value = {0};
        String s = "";

        if (mBluetoothLeService == null) {
            Log.w(TAG, "code or bt service is null");
            return;
        }

        Date now = new Date();
        long timestamp = now.getTime();
        SimpleDateFormat sdf = new SimpleDateFormat("yyMMddHHmm");
        String dateStr = sdf.format(timestamp);

        s = Integer.toString(mBluetoothLeService.SL_RESET_CMD) + phone_id + dateStr;
        Log.w(TAG, "Send Reset CMD = " + s);

        value = s.getBytes();

        updateTheTextView(" <font color=\\\"#FE6026\\\">" + "Reset Locker" + "</font>");

        mBluetoothLeService.writeCustomCharacteristic(value);

        UpdateLockStatus(0xD0);
    }

    public void OnClickResultView(View v) {
        TextView result = (TextView) findViewById(R.id.result_view);
        result.setText("");
    }

    public void showAlert(String title, String context)
    {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(title);
        alert.setMessage(context);
        alert.show();
    }

    public static void UpdateLockStatus(int lock) {
        if (lock == 0xD0) {
            lock_view.setText(Html.fromHtml("Lock status : " + "<font color=\"#FF0000\">" + "Lock" + "</font>"));
            lock_status = SMARTLOCKER_LOCKED;
        } else if (lock == 0xD1){
            lock_view.setText(Html.fromHtml("Lock status : " + "<font color=\"#0DFF00\">" + "Unlock" + "</font>"));
            lock_status = SMARTLOCKER_UNLOCKED;
        } else if (lock == 0xD2) {
            lock_view.setText(Html.fromHtml("Lock status : " + "<font color=\"#FF0000\">" + "Unusable" + "</font>"));
            lock_status = SMARTLOCKER_UNUSABLE;
        }
    }

    public static DeviceControlActivity getInstace() {
        return ins;
    }

    public void updateTheTextView(final String s) {
        DeviceControlActivity.this.runOnUiThread(new Runnable() {
            public void run() {
                TextView result = (TextView) findViewById(R.id.result_view);
                result.setMovementMethod(new ScrollingMovementMethod());
                result.append(Html.fromHtml("[" + mTimeFormat.format(new Date()) + "]" + ": " + s));
                result.append("\n");
                int offset= result.getLineCount()*result.getLineHeight();
                if (offset > result.getHeight()) {
                    result.scrollTo(0, offset - result.getHeight());
                }
            }
        });
    }
}
