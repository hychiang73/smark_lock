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

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Binder;
import android.os.IBinder;
import android.text.Html;
import android.util.Log;

import java.util.List;
import java.util.UUID;

/**
 * Service for managing connection and data communication with a GATT server hosted on a
 * given Bluetooth LE device.
 */
public class BluetoothLeService extends Service {
    private final static String TAG = BluetoothLeService.class.getSimpleName();

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;
    private String mBluetoothDeviceAddress;
    private BluetoothGatt mBluetoothGatt;
    public int mConnectionState = STATE_DISCONNECTED;

    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_CONNECTED = 2;

    public final int SL_UNLOCK_CMD = 0x0;
    public final int SL_LOCK_CMD = 0x1;
    public final int SL_UPDATE_CODE_CMD = 0x2;
    public final int SL_APP_READY_CMD = 0x3;
    public final int SL_RESET_CMD = 0x4;

    public final int SL_LOCK_SUCCESS = 0x10;
    public final int SL_UNLOCK_SUCCESS = 0x11;
    public final int SL_UPDATE_SUCCESS = 0x12;
    public final int SL_RESET_SUCCESS = 0x13;
    public final int SL_DEV_NEED_UPDATE = 0x14;
    public final int SL_CODE_RUN_OUT = 0x15;
    public final int SL_UNLOCK_FAIL = 0x16;
    public final int SL_LOCK_FAIL = 0x17;
    public final int SL_CODE_OUT_OF_DATE = 0x18;
    public final int SL_CODE_INVALID = 0x19;
    public final int SL_DEV_ID_FAIL = 0x1A;

    public final int SL_LOCK = 0xD0;
    public final int SL_UNLOCK = 0xD1;
    public final int SL_UNUSABLE = 0xD2;

    public final static String ACTION_GATT_CONNECTED =
            "com.example.bluetooth.le.ACTION_GATT_CONNECTED";
    public final static String ACTION_GATT_DISCONNECTED =
            "com.example.bluetooth.le.ACTION_GATT_DISCONNECTED";
    public final static String ACTION_GATT_SERVICES_DISCOVERED =
            "com.example.bluetooth.le.ACTION_GATT_SERVICES_DISCOVERED";
    public final static String ACTION_DATA_AVAILABLE =
            "com.example.bluetooth.le.ACTION_DATA_AVAILABLE";
    public final static String EXTRA_DATA =
            "com.example.bluetooth.le.EXTRA_DATA";

    private static DeviceControlActivity mDeviceControl = new DeviceControlActivity();

    // Implements callback methods for GATT events that the app cares about.  For example,
    // connection change and services discovered.
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            String intentAction;
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                intentAction = ACTION_GATT_CONNECTED;
                mConnectionState = STATE_CONNECTED;
                broadcastUpdate(intentAction);
                Log.i(TAG, "Connected to GATT server.");
                // Attempts to discover services after successful connection.
                Log.i(TAG, "Attempting to start service discovery:" +
                        mBluetoothGatt.discoverServices());

            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                intentAction = ACTION_GATT_DISCONNECTED;
                mConnectionState = STATE_DISCONNECTED;
                Log.i(TAG, "Disconnected from GATT server.");
                broadcastUpdate(intentAction);
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            Log.w(TAG, "onServicesDiscovered");
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_GATT_SERVICES_DISCOVERED);
            } else {
                Log.w(TAG, "onServicesDiscovered received: " + status);
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            Log.w(TAG, "onCharacteristicRead");
            Log.w(TAG, "Read data = " + characteristic.getUuid() + ", " + characteristic.getValue());
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            Log.w(TAG, "onCharacteristicChanged");
            //Log.w(TAG, "Read data = " + characteristic.getUuid() + ", " + characteristic.getValue());
            broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
        }
    };

    private void broadcastUpdate(final String action) {
        final Intent intent = new Intent(action);
        sendBroadcast(intent);
    }

    private void broadcastUpdate(final String action,
                                 final BluetoothGattCharacteristic characteristic) {
        final Intent intent = new Intent(action);
        Log.w(TAG, "broadcastUpdate called by onCharacteristicRead");

        int flag = characteristic.getProperties();
        int format = -1;
        if ((flag & 0x01) != 0) {
            format = BluetoothGattCharacteristic.FORMAT_UINT16;
            Log.d(TAG, "Heart rate format UINT16.");
        } else {
            format = BluetoothGattCharacteristic.FORMAT_UINT8;
            Log.d(TAG, "Heart rate format UINT8.");
        }

        final int access_code = characteristic.getIntValue(format, 0);
        Log.d(TAG, String.format("Received code: %d", access_code));

        ParseSmartLockCode(access_code);

        sendBroadcast(intent);
    }

    public class LocalBinder extends Binder {
        BluetoothLeService getService() {
            return BluetoothLeService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        // After using a given device, you should make sure that BluetoothGatt.close() is called
        // such that resources are cleaned up properly.  In this particular example, close() is
        // invoked when the UI is disconnected from the Service.
        close();
        return super.onUnbind(intent);
    }

    private final IBinder mBinder = new LocalBinder();

    /**
     * Initializes a reference to the local Bluetooth adapter.
     *
     * @return Return true if the initialization is successful.
     */
    public boolean initialize() {
        // For API level 18 and above, get a reference to BluetoothAdapter through
        // BluetoothManager.
        if (mBluetoothManager == null) {
            mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
            if (mBluetoothManager == null) {
                Log.e(TAG, "Unable to initialize BluetoothManager.");
                return false;
            }
        }

        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Unable to obtain a BluetoothAdapter.");
            return false;
        }

        return true;
    }

    /**
     * Connects to the GATT server hosted on the Bluetooth LE device.
     *
     * @param address The device address of the destination device.
     *
     * @return Return true if the connection is initiated successfully. The connection result
     *         is reported asynchronously through the
     *         {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     *         callback.
     */
    public boolean connect(final String address) {
        if (mBluetoothAdapter == null || address == null) {
            Log.w(TAG, "BluetoothAdapter not initialized or unspecified address.");
            return false;
        }

        // Previously connected device.  Try to reconnect.
        if (mBluetoothDeviceAddress != null && address.equals(mBluetoothDeviceAddress)
                && mBluetoothGatt != null) {
            Log.d(TAG, "Trying to use an existing mBluetoothGatt for connection.");
            if (mBluetoothGatt.connect()) {
                mConnectionState = STATE_CONNECTING;
                return true;
            } else {
                return false;
            }
        }

        final BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address);
        if (device == null) {
            Log.w(TAG, "Device not found.  Unable to connect.");
            return false;
        }
        // We want to directly connect to the device, so we are setting the autoConnect
        // parameter to false.
        mBluetoothGatt = device.connectGatt(this, false, mGattCallback);
        Log.d(TAG, "Trying to create a new connection.");
        mBluetoothDeviceAddress = address;
        mConnectionState = STATE_CONNECTING;
        return true;
    }

    /**
     * Disconnects an existing connection or cancel a pending connection. The disconnection result
     * is reported asynchronously through the
     * {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     * callback.
     */
    public void disconnect() {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.disconnect();
    }

    /**
     * After using a given BLE device, the app must call this method to ensure resources are
     * released properly.
     */
    public void close() {
        if (mBluetoothGatt == null) {
            return;
        }
        mBluetoothGatt.close();
        mBluetoothGatt = null;
    }

    /**
     * Request a read on a given {@code BluetoothGattCharacteristic}. The read result is reported
     * asynchronously through the {@code BluetoothGattCallback#onCharacteristicRead(android.bluetooth.BluetoothGatt, android.bluetooth.BluetoothGattCharacteristic, int)}
     * callback.
     *
     * @param characteristic The characteristic to read from.
     */
    public void readCharacteristic(BluetoothGattCharacteristic characteristic) {
        Log.w(TAG, "readCharacteristic");
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.readCharacteristic(characteristic);
    }

    /**
     * Enables or disables notification on a give characteristic.
     *
     * @param characteristic Characteristic to act on.
     * @param enabled If true, enable notification.  False otherwise.
     */
    public void setCharacteristicNotification(BluetoothGattCharacteristic characteristic,
                                              boolean enabled) {
        Log.w(TAG, "setCharacteristicNotification");
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }

        mBluetoothGatt.setCharacteristicNotification(characteristic, enabled);
        BluetoothGattDescriptor descriptor = characteristic.getDescriptor(UUID.fromString(SampleGattAttributes.CLIENT_CHARACTERISTIC_CONFIG));
        descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        mBluetoothGatt.writeDescriptor(descriptor);
    }

    public void SetCharNotification() {
        BluetoothGattService mCustomService = mBluetoothGatt.getService(UUID.fromString("8653000a-43e6-47b7-9cb0-5fc21d4ae340"));
        Log.w(TAG, "mCustomService = " + mCustomService);
        BluetoothGattCharacteristic characteristic = mCustomService.getCharacteristic(UUID.fromString("8653000b-43e6-47b7-9cb0-5fc21d4ae340"));
        mBluetoothGatt.setCharacteristicNotification(characteristic, true);
        Log.w(TAG, "characteristic = " + characteristic.getUuid().toString());
        BluetoothGattDescriptor descriptor = characteristic.getDescriptor(UUID.fromString(SampleGattAttributes.CLIENT_CHARACTERISTIC_CONFIG));
        Log.w(TAG, "descriptor = " + descriptor.getUuid().toString());
        descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        mBluetoothGatt.writeDescriptor(descriptor);
    }

    /**
     * Retrieves a list of supported GATT services on the connected device. This should be
     * invoked only after {@code BluetoothGatt#discoverServices()} completes successfully.
     *
     * @return A {@code List} of supported services.
     */
    public List<BluetoothGattService> getSupportedGattServices() {
        if (mBluetoothGatt == null) return null;

        return mBluetoothGatt.getServices();
    }

    public void writeCustomCharacteristic(byte[] value) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        /*check if the service is available on the device*/
        BluetoothGattService mCustomService = mBluetoothGatt.getService(UUID.fromString("8653000a-43e6-47b7-9cb0-5fc21d4ae340"));
        if(mCustomService == null){
            Log.w(TAG, "Custom BLE Service not found");
            return;
        }
        /*get the read characteristic from the service*/
        BluetoothGattCharacteristic mWriteCharacteristic = mCustomService.getCharacteristic(UUID.fromString("8653000c-43e6-47b7-9cb0-5fc21d4ae340"));
        //mWriteCharacteristic.setValue(value, android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT8,0);
        mWriteCharacteristic.setValue(value);
        if(mBluetoothGatt.writeCharacteristic(mWriteCharacteristic) == false){
            Log.w(TAG, "Failed to write characteristic");
        }
    }

    public void ParseSmartLockCode (int result_code) {

        Log.w(TAG, "ParseSmartLockCode = " + result_code);

        switch(result_code) {
            case SL_LOCK_SUCCESS:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#0DFF00\">" + "Lock successful !" + "</font>");
                DeviceControlActivity.UpdateLockStatus(SL_LOCK);
                return;
            case SL_UNLOCK_SUCCESS:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#0DFF00\">" + "Unlock successful !" + "</font>");
                DeviceControlActivity.UpdateLockStatus(SL_UNLOCK);
                break;
            case SL_UPDATE_SUCCESS:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#0DFF00\">" + "Update access codes successfully !" + "</font>");
                break;
            case SL_RESET_SUCCESS:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#0DFF00\">" + "Reset done !" + "</font>");
                break;
            case SL_UNLOCK_FAIL:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#FF0000\">" + "Out of Sync! Update required" + "</font>");
                break;
            case SL_LOCK_FAIL:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#FF0000\">" + "WARNING ! Locker is not secured !" + "</font>");
                DeviceControlActivity.UpdateLockStatus(SL_UNUSABLE);
                break;
            case SL_CODE_INVALID:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#FF0000\">" + "Invalid commands." + "</font>");
                break;
            case SL_DEV_NEED_UPDATE:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#FF0000\">" + "Locker has no codes inside. Update required" + "</font>");
                break;
            case SL_CODE_RUN_OUT:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#FF0000\">" + "Codes are running out. Locker needs new codes" + "</font>");
                break;
            case SL_CODE_OUT_OF_DATE:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#FF0000\">" + "Code is outdated. Update required." + "</font>");
                break;
            case SL_DEV_ID_FAIL:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#FF0000\">" + "Phone ID is not correct." + "</font>");
                break;
            case SL_LOCK:
                DeviceControlActivity.UpdateLockStatus(SL_LOCK);
                break;
            case SL_UNLOCK:
                DeviceControlActivity.UpdateLockStatus(SL_UNLOCK);
                break;
            case SL_UNUSABLE:
                DeviceControlActivity.UpdateLockStatus(SL_UNUSABLE);
                break;
            default:
                mDeviceControl.getInstace().updateTheTextView("<font color=\"#FF0000\">" + "Unknown values"  + "</font>");
                return;
        }
    }
}
