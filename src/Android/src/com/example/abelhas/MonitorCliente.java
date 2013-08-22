package com.example.abelhas;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import ufrj.coppe.lcp.repa.PrefixAddress;
import ufrj.coppe.lcp.repa.RepaMessage;
import ufrj.coppe.lcp.repa.RepaSocket;
import ufrj.coppe.lcp.repa.RepaSocketException;

public class MonitorCliente {
	private RepaSocket rpa = new RepaSocket();
	private Thread sendMsg;
	private Handler handler;
	private Message message = new Message();
	
	private void log(String s){
		message = new Message();
		message.obj = s;
		handler.sendMessage(message);	
	}
	
	public MonitorCliente(Handler handler) {
		this.handler = handler;
		try {
			initRepa();
			
			sendMsg = new Thread(new Runnable() {
				
				@Override
				public void run() {
					// TODO Auto-generated method stub
					sendMsgToServer();
				}
			});
			sendMsg.setPriority(Thread.MAX_PRIORITY);
			sendMsg.start();
			
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	private void initRepa() {
		try {
			rpa.repaOpen();
			rpa.registerInterest("client");
			log("Setado interesse em client");
		} catch (Exception e) {
			// TODO!
			e.printStackTrace();
		}
	}

	private void closeRepa(){
		try {
			rpa.repaClose();
		} catch (RepaSocketException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public void close(){
		sendMsg.stop();
		closeRepa();
	}

	int i = 0;
	private String getTemperature() {
		i+=0.1;
		return String.valueOf((int) ((Math.sin(i)*10+26) + Math.random() * 5 - 1) + " graus celcius.");
	}

	private void sendMsgToServer() {
		while (true){
			String data = getTemperature();
			String interest = "server";
			int data_length = data.length();
			if (data_length > 0) {
				try {
					rpa.repaSend(new RepaMessage(interest, data.getBytes(),
						new PrefixAddress()));
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				Log.i("Client","Message sent I: "+interest+" | D: "+data+"\n");
				log("Mensagem enviada I: "+interest+" | D: "+data+"\n");
			}
			try {
				Thread.sleep(10000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
}
