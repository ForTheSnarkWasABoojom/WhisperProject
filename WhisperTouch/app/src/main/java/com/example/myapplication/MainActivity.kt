package com.example.myapplication

import android.Manifest
import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothSocket
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.speech.RecognizerIntent
import android.util.Log
import android.view.View
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.ListView
import android.widget.RadioGroup
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresPermission
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.io.IOException
import java.io.OutputStream
import java.util.*

class MainActivity : AppCompatActivity() {
    private var btAdapter: BluetoothAdapter? = null
    private lateinit var listView: ListView
    private lateinit var deviceList: ArrayList<String>
    private lateinit var devices: ArrayList<BluetoothDevice>
    private lateinit var etMessage: EditText
    private var speed: Float = 1.0f
    private lateinit var speedGroup: RadioGroup

    private var selectedDevice: BluetoothDevice? = null
    private var bluetoothSocket: BluetoothSocket? = null
    private var outputStream: OutputStream? = null

    val result = registerForActivityResult(ActivityResultContracts.StartActivityForResult()){
            result->
        if (result.resultCode == Activity.RESULT_OK){
            val results = result.data?.getStringArrayListExtra(
                RecognizerIntent.EXTRA_RESULTS
            ) as ArrayList<String>

            etMessage.setText(results[0])
        }
    }

    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        listView = findViewById(R.id.listView)
        etMessage = findViewById(R.id.etMessage)
        speedGroup = findViewById(R.id.speedGroup)
        deviceList = ArrayList()
        devices = ArrayList()
        
        speed = 1.0f
        speedGroup.check(R.id.speed3)

        speedGroup.setOnCheckedChangeListener { _, checkedId ->
            speed = when (checkedId) {
                R.id.speed1 -> 0.5f
                R.id.speed2 -> 0.75f
                R.id.speed3 -> 1.0f
                R.id.speed4 -> 1.25f
                R.id.speed5 -> 1.5f
                else -> 1.0f
            }
        }
        
        val btnSend = findViewById<Button>(R.id.btnSend)
        btnSend.setOnClickListener {
            val message = etMessage.text.toString()
            if (message.isNotEmpty()) {
                sendMessage(message)
            } else {
                Toast.makeText(this, "Введите сообщение", Toast.LENGTH_SHORT).show()
            }
        }
        
        listView.setOnItemClickListener { _, _, position, _ ->
            if (position < devices.size) {
                selectedDevice = devices[position]
                connectToDevice()
            }
        }

        val speechToTextBtn = findViewById<Button>(R.id.btnSearch)
        speechToTextBtn.setOnClickListener {
            etMessage.text = null
            try {
                val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH)
                intent.putExtra(
                    RecognizerIntent.EXTRA_LANGUAGE_MODEL,
                    RecognizerIntent.LANGUAGE_MODEL_FREE_FORM
                )
                intent.putExtra(
                    RecognizerIntent.EXTRA_LANGUAGE,
                    Locale.getDefault()
                )
                intent.putExtra(RecognizerIntent.EXTRA_PROMPT,"Say something")
                result.launch(intent)
            }catch (e:Exception){
                e.printStackTrace()
            }
        }
        init()
    }

    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    private fun init() {
        val btManager = getSystemService(BLUETOOTH_SERVICE) as BluetoothManager
        btAdapter = btManager.adapter
        getPairedDevices()
    }

    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    private fun getPairedDevices() {
        deviceList.clear()
        devices.clear()
        val pairedDevices: Set<BluetoothDevice>? = btAdapter?.bondedDevices
        pairedDevices?.forEach {
            deviceList.add("${it.name} (${it.address})")
            devices.add(it)
        }
        
        if (deviceList.isEmpty()) {
            deviceList.add("No devices found")
        }
        
        val adapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, deviceList)
        listView.adapter = adapter
    }

    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    private fun connectToDevice() {
        if (selectedDevice == null) {
            Toast.makeText(this, "No device selected", Toast.LENGTH_SHORT).show()
            return
        }

        try {
            val uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
            bluetoothSocket = selectedDevice?.createRfcommSocketToServiceRecord(uuid)
            bluetoothSocket?.connect()
            outputStream = bluetoothSocket?.outputStream
            Toast.makeText(this, "Connected to ${selectedDevice?.name}", Toast.LENGTH_SHORT).show()
        } catch (e: IOException) {
            Toast.makeText(this, "Connection failed: ${e.message}", Toast.LENGTH_SHORT).show()
            e.printStackTrace()
        }
    }

    private fun sendMessage(message: String) {
        if (outputStream == null) {
            Toast.makeText(this, "Not connected to any device", Toast.LENGTH_SHORT).show()
            return
        }

        try {
            val jsonMessage = "{\"text\":\"$message\",\"speed\":\"%.2f\"}".format(speed)

            outputStream?.write(jsonMessage.toByteArray())
            Toast.makeText(this, "Message sent: $message", Toast.LENGTH_SHORT).show()
            etMessage.text.clear()
        } catch (e: IOException) {
            Toast.makeText(this, "Failed to send message: ${e.message}", Toast.LENGTH_SHORT).show()
            e.printStackTrace()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        try {
            bluetoothSocket?.close()
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }
}