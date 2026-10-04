
package com.drbeef.lambda1vr;

import static android.system.Os.setenv;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;

import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.view.KeyEvent;

import static android.system.Os.setenv;


@SuppressLint("SdCardPath") public class GLES3JNIActivity extends Activity implements SurfaceHolder.Callback
{
	private static String manufacturer = "";

	static
	{
		manufacturer = Build.MANUFACTURER.toLowerCase(Locale.ROOT);
		if (manufacturer.contains("oculus")) // rename oculus to meta as this will probably happen in the future anyway
		{
			manufacturer = "meta";
		}

		if (BuildConfig.STEAM_FRAME)
		{
			//The Frame has no loader broker, so the Khronos loader is in the APK
			System.loadLibrary("openxr_loader");
			try
			{
				setenv("OPENXR_HMD", "steamframe", true);
			} catch (Exception e)
			{}
		}
		else
		{
			try
			{
				//Load manufacturer specific loader
				System.loadLibrary("openxr_loader_" + manufacturer);
				setenv("OPENXR_HMD", manufacturer, true);
			} catch (Exception e)
			{}
		}

		System.loadLibrary( "xash" );
	}

	private static final String TAG = "Lambda1VR";

	//Where the game data lives. On the Frame it's in Documents, which is the
	//headset's own folder and survives a reset of the Android container.
	private static final String DATA_DIR = BuildConfig.STEAM_FRAME ? "/sdcard/Documents/Lambda1VR/" : "/sdcard/xash/";

	private static final int REQUEST_MANAGE_ALL_FILES = 2296;


	String commandLineParams;

	private SurfaceView mView;
	private SurfaceHolder mSurfaceHolder;
	private long mNativeHandle;

	public void shutdown() {
		System.exit(0);
	}

	@Override protected void onCreate( Bundle icicle )
	{
		Log.v( TAG, "----------------------------------------------------------------" );
		Log.v( TAG, "GLES3JNIActivity::onCreate()" );
		super.onCreate( icicle );

		mView = new SurfaceView( this );
		setContentView( mView );
		mView.getHolder().addCallback( this );

		checkPermissionsAndInitialize();
	}

	/** Initializes the Activity only if the permission has been granted. */
	private boolean canUseStorage() {
		if (Environment.isExternalStorageManager()) {
			return true;
		}

		if (BuildConfig.STEAM_FRAME) {
			//The container on the Frame has no settings page for this, and the folder is
			//plain files from the host, so all that matters is if we can write to it
			File dir = new File(DATA_DIR);
			return (dir.isDirectory() || dir.mkdirs()) && dir.canWrite();
		}

		return false;
	}

	private void checkPermissionsAndInitialize() {
		if (!canUseStorage()) {
			//request for the permission
			Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
			Uri uri = Uri.fromParts("package", getPackageName(), null);
			intent.setData(uri);
			try {
				startActivityForResult(intent, REQUEST_MANAGE_ALL_FILES);
			} catch (ActivityNotFoundException e) {
				Log.e(TAG, "No settings page for the all files permission, and " + DATA_DIR + " can't be written");
			}

			finishAffinity(); // Cleanly exit

		}
		else
		{
			// Permissions have already been granted.
			create();
		}
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		finishAffinity(); // Cleanly exit
		System.exit(0);
	}

	/**
	 * The Frame's Android container shows the headset's Steam folders at the same paths, read only. If
	 * Half-Life is installed there, the game's files can be used from where they are instead of a copy.
	 * Looks in the Steam library folders (the main one, and the others libraryfolders.vdf lists, such as
	 * one on an SD card) for steamapps/common/Half-Life/valve/liblist.gam. Returns the Half-Life folder or null.
	 */
	private String findSteamHalfLife()
	{
		final String steam = "/home/steamos/.local/share/Steam";
		java.util.ArrayList<String> libraries = new java.util.ArrayList<String>();
		libraries.add(steam);

		try
		{
			BufferedReader br = new BufferedReader(new FileReader(steam + "/steamapps/libraryfolders.vdf"));
			String line;
			while ((line = br.readLine()) != null)
			{
				// "path"		"/some/library"
				line = line.trim();
				if (line.startsWith("\"path\""))
				{
					String[] parts = line.split("\"");
					if (parts.length >= 4 && !libraries.contains(parts[3]))
					{
						libraries.add(parts[3]);
					}
				}
			}
			br.close();
		} catch (IOException e)
		{
			Log.i(TAG, "[data] no libraryfolders.vdf: " + e.getMessage());
		}

		for (String library : libraries)
		{
			File liblist = new File(library + "/steamapps/common/Half-Life/valve/liblist.gam");
			if (liblist.canRead())
			{
				return library + "/steamapps/common/Half-Life";
			}
			Log.i(TAG, "[data] no Half-Life in the Steam library " + library);
		}
		return null;
	}

	public void create()
	{
		copy_asset(getFilesDir().getPath(), "extras.pak", false);
		copy_asset(DATA_DIR, "commandline.txt", false); // Copy in case user has deleted their config


		//Read these from a file and pass through
		commandLineParams = new String("xash3d -dev 3 -log");

		//See if user is trying to use command line params
		if(new File(DATA_DIR + "commandline.txt").exists()) // should exist!
		{
			BufferedReader br;
			try {
				br = new BufferedReader(new FileReader(DATA_DIR + "commandline.txt"));
				String s;
				StringBuilder sb=new StringBuilder(0);
				while ((s=br.readLine())!=null)
					sb.append(s + " ");
				br.close();

				commandLineParams = new String(sb.toString());
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

		String[] params = commandLineParams.split(" ");
		
		String game = "valve";
		
		int i = 0;
		for (i = 0; i < params.length; ++i)
		{
			if (params[i].compareTo("-game") == 0)
			{
				game = params[i+1];
			}
		}

		//game configuration
		copy_asset(DATA_DIR + game + "/", "config.cfg", false); // Copy in case user has deleted their config

		//special commands
		copy_asset(DATA_DIR + game + "/", "commands.lst", false); // Copy in case user has deleted their config

		//Copy our special stuff
		copy_asset(DATA_DIR + game + "/", "sprites/s_stealth.spr", true);
		copy_asset(DATA_DIR + game + "/", "sprites/vignette.tga", true);
		copy_asset(DATA_DIR + "valve/", "sprites/vignette.tga", true); //seems to need to be here for some people

		//Menu Arrow
		copy_asset(DATA_DIR + game + "/", "sprites/pointer.tga", true);
		copy_asset(DATA_DIR + "valve/", "sprites/pointer.tga", true);

		//Menu Background
		copy_asset(DATA_DIR + "valve/resource/", "background/800_1_a_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_1_b_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_1_c_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_1_d_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_2_a_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_2_b_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_2_c_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_2_d_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_3_a_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_3_b_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_3_c_loading.tga", true);
		copy_asset(DATA_DIR + "valve/resource/", "background/800_3_d_loading.tga", true);


		//Copy modified weapon models - This is the base set
		if (!(new File(DATA_DIR + game + "/models/no_copy").exists()) && !(game.equalsIgnoreCase("Hunger")))
		{
			//Colt (A-16)
			copy_asset(DATA_DIR + game + "/", "models/v_9mmar.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_9mmar.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_9mmar.mdl", true);
			copy_asset(DATA_DIR + game + "/", "sound/items/clipinsert1.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/items/cliprelease1.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/t_m4_boltpull.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/t_m4_deploy.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/t_m4_m203_in.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/t_m4_m203_out.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/t_m4_m203_shell.wav", true);

			//Colt Pistol M1911
			copy_asset(DATA_DIR + game + "/", "models/v_9mmhandgun.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_9mmhandgun.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_9mmhandgun.mdl", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/glock_magin.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/glock_magout.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/glock_slideforward.wav", true);

			//357 Python
			copy_asset(DATA_DIR + game + "/", "models/v_357.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_357.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_357.mdl", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/357_shot1.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/357_shot2.wav", true);

			//RGD Grenade
			copy_asset(DATA_DIR + game + "/", "models/v_grenade.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_grenade.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_grenade.mdl", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/grenade_pinpull.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/grenade_throw.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/grenade_draw.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/explode3.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/explode4.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/explode5.wav", true);

			//Shotgun Benelli M3
			copy_asset(DATA_DIR + game + "/", "models/v_shotgun.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_shotgun.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_shotgun.mdl", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/m3_insertshell.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/m3_pump.wav", true);

			//Stalker Rocket
			copy_asset(DATA_DIR + game + "/", "models/v_rpg.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_rpg.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_rpg.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/rpgrocket.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/v_rpgammo.mdl", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/rocket1.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/rocketfire1.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/rpg7_reload.wav", true);

			//Satchel
			copy_asset(DATA_DIR + game + "/", "models/v_satchel.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/v_satchel_radio.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_satchel.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_satchel.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_satchel_radio.mdl", true);

			//Tripmine
			copy_asset(DATA_DIR + game + "/", "models/v_tripmine.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_tripmine.mdl", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/mine_activate.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/mine_deploy.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/tripmine_ant.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/tripmine_button.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/tripmine_move.wav", true);

			//BMS Crossbow
			copy_asset(DATA_DIR + game + "/", "models/v_crossbow.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_crossbow.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_crossbow.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_crossbow_clip.mdl", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/natianul.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/xbow.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/xbow_fire1.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/xbow_reload1.wav", true);


			//Egon (Overhaul)
			copy_asset(DATA_DIR + game + "/", "models/v_egon.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_egon.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_egon.mdl", true);

			//Gauss (Overhaul)
			copy_asset(DATA_DIR + game + "/", "models/v_gauss.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_gauss.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_gauss.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_gaussammo.mdl", true);

			//HGun (Overhaul)
			copy_asset(DATA_DIR + game + "/", "models/v_hgun.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_hgun.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_hgun.mdl", true);

			//Squeak (Overhaul)
			copy_asset(DATA_DIR + game + "/", "models/v_squeak.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/p_squeak.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_squeak.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_sqknest.mdl", true);

			//Crowbar
			copy_asset(DATA_DIR + game + "/", "models/v_crowbar.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/w_crowbar.mdl", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/cbar_draw.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/cbar_hit1.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/cbar_hit2.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/cbar_hitbod1.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/cbar_hitbod2.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/cbar_hitbod3.wav", true);
			copy_asset(DATA_DIR + game + "/", "sound/weapons/cbar_miss1.wav", true);

			copy_asset(DATA_DIR + game + "/", "models/v_torch.mdl", true);
			copy_asset(DATA_DIR + game + "/", "models/v_hand.mdl", true);
		}

		//Copy Opposing Force specific models
		if (game.equalsIgnoreCase("gearbox") &&
				!(new File(DATA_DIR + game + "/models/no_copy").exists()))
		{
			//Scope vignette texture
			copy_asset(DATA_DIR + game + "/", "sprites/scope.tga", true);
			copy_asset(DATA_DIR + "valve/", "sprites/scope.tga", true);

			//Sniper Rifle
			copy_asset(DATA_DIR, "gearbox/models/v_m40a1.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/w_m40a1.mdl", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/scout_clipin.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/scout_clipout.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/sniper_fire.wav", true);

			//Pipe Wrench
			copy_asset(DATA_DIR, "gearbox/models/v_pipe_wrench.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/w_pipe_wrench.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/p_pipe_wrench.mdl", true);

			//Penguins
			new File(DATA_DIR + "gearbox/sound/penguin/").mkdirs(); // Make the penguin directory as it won't exist
			copy_asset(DATA_DIR, "gearbox/sound/penguin/penguin_die1.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/penguin/penguin_deploy1.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/penguin/penguin_hunt1.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/penguin/penguin_hunt2.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/penguin/penguin_hunt3.wav", true);

			//Knife
			copy_asset(DATA_DIR, "gearbox/models/v_knife.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/w_knife.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/p_knife.mdl", true);

			//Desert Eagle
			copy_asset(DATA_DIR, "gearbox/models/v_desert_eagle.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/w_desert_eagle.mdl", true);

			//Saw
			copy_asset(DATA_DIR, "gearbox/models/v_saw.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/p_saw.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/w_saw.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/w_saw_clip.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/saw_link.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/saw_shell.mdl", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/SAW_bolt.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/saw_fire1.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/saw_fire2.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/saw_fire3.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/saw_reload_new.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/saw_reload2_new.wav", true);

			//Barnacle
			copy_asset(DATA_DIR, "gearbox/models/v_bgrap.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/p_bgrap.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/w_bgrap.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/v_bgrap_tonguetip.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/saw_link.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/saw_shell.mdl", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/bgrapple_cough.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/bgrapple_fire.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/bgrapple_impact.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/bgrapple_pull.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/bgrapple_release.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/bgrapple_wait.wav", true);

			//Shock
			copy_asset(DATA_DIR, "gearbox/models/v_shock.mdl", true);

			//Pingu
			copy_asset(DATA_DIR, "gearbox/models/v_penguin.mdl", true);

			//Spore
			copy_asset(DATA_DIR, "gearbox/models/v_spore_launcher.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/w_spore_launcher.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/spore.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/spore_ammo.mdl", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/splauncher_bounce.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/splauncher_impact.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/spore_hit1.wav", true);

			//Displacer
			copy_asset(DATA_DIR, "gearbox/models/v_displacer.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/w_displacer.mdl", true);
			copy_asset(DATA_DIR, "gearbox/models/p_displacer.mdl", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/displacer_fire.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/displacer_impact.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/displacer_self.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/displacer_spin.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/displacer_spin2.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/displacer_start.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/displacer_teleport.wav", true);
			copy_asset(DATA_DIR, "gearbox/sound/weapons/displacer_teleport_player.wav", true);

			//Hand
			copy_asset(DATA_DIR, "gearbox/models/v_hand.mdl", true);
		}

		//Copy Blue Shift specific models
		if (game.equalsIgnoreCase("bshift") &&
				!(new File(DATA_DIR + game + "/models/no_copy").exists()))
		{
			copy_asset(DATA_DIR, "bshift/models/v_hand.mdl", true);
		}
		
		//Copy They Hunger specific models
		if (game.equalsIgnoreCase("Hunger") &&
				!(new File(DATA_DIR + game + "/models/no_copy").exists()))
		{
			//HL1 models
			copy_asset(DATA_DIR + game + "/", "/models/v_rpg.mdl", true);
			copy_asset(DATA_DIR + game + "/", "/models/v_357.mdl", true);
			copy_asset(DATA_DIR + game + "/", "/models/v_satchel.mdl", true);
			copy_asset(DATA_DIR + game + "/", "/models/v_satchel_radio.mdl", true);
			copy_asset(DATA_DIR + game + "/", "/models/v_tripmine.mdl", true);
			copy_asset(DATA_DIR + game + "/", "/models/v_crossbow.mdl", true);
			copy_asset(DATA_DIR + game + "/", "/models/v_gauss.mdl", true);
			copy_asset(DATA_DIR + game + "/", "/models/v_squeak.mdl", true);
			
			//They Hunger models
			copy_asset(DATA_DIR, "Hunger/models/v_9mmar.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_9mmhandgun.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_ap9.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_crowbar.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_egon.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_hkg36.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_shotgun.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_shovel.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_taurus.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_tfac.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_tfc_medkit.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_tfc_sniper.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_tfc_spanner.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_tnt.mdl", true);
			copy_asset(DATA_DIR, "Hunger/models/v_hand.mdl", true);
		}
		
		//Copy Afraid of Monsters Director's Cut specific models
		if (game.equalsIgnoreCase("AoMDC") &&
				!(new File(DATA_DIR + game + "/models/no_copy").exists()))
		{
			//Afraid of Monsters Director's Cut models
			copy_asset(DATA_DIR, "AoMDC/models/v_axe.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_beretta.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_deagle.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_glock.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_hammer.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_hand.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_kitchenknife.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_mp5k.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_p228.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_revolver.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_shotgun.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_spear.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_torch.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/v_uzi.mdl", true);
			copy_asset(DATA_DIR, "AoMDC/models/gmgeneral_display.aomdc", true);
		}

		//Set default environment
		try {
			ApplicationInfo info = getApplicationInfo();
			setenv("XASH3D_BASEDIR", DATA_DIR, true);

			if (BuildConfig.STEAM_FRAME)
			{
				// Half-Life from Steam, read only, in place. The folder above is searched after it, so what's
				// in there (the saves, the settings, this app's own models and sprites) wins over Steam's files.
				String steamHalfLife = findSteamHalfLife();
				if (steamHalfLife != null)
				{
					setenv("XASH3D_RODIR", steamHalfLife, true);
					Log.i(TAG, "[data] game files: Steam, in place, read only (" + steamHalfLife + "); saves and settings in " + DATA_DIR);
				}
				else
				{
					Log.i(TAG, "[data] game files: no Steam install of Half-Life found, using the copy in " + DATA_DIR);
				}
			}
			setenv("XASH3D_GAMELIBDIR", info.nativeLibraryDir, true);
			setenv("XASH3D_GAMEDIR", "valve", true);
			setenv( "XASH3D_EXTRAS_PAK1", getFilesDir().getPath() + "/extras.pak", true );

			//If game is gearbox (Opposing Force) set the library file suffix to opfor
			if (game.equalsIgnoreCase("gearbox"))
			{
				setenv("XASH3D_LIBSUFFIX", "_opfor", true);

				//Use pipewrench as backpack weapon in opposing force
				setenv("VR_BACKPACK_WEAPON", "weapon_pipewrench", true);
			}
			else if (game.equalsIgnoreCase("bshift"))
			{
				setenv("XASH3D_LIBSUFFIX", "_bshift", true);
			}
			else if (game.equalsIgnoreCase("aomdc"))
			{
				setenv("XASH3D_LIBSUFFIX", "_aomdc", true);
			}
			else if (game.equalsIgnoreCase("hunger"))
			{
				setenv("XASH3D_LIBSUFFIX", "_theyhunger", true);
			}
			else
			{
				setenv("XASH3D_LIBSUFFIX", "", true);
			}
		}
		catch (Exception e)
		{

		}
		
		mNativeHandle = GLES3JNILib.onCreate( this, commandLineParams );
	}
	
	public void copy_asset(String path, String name, boolean forceOverwrite) {
		File f = new File(path + "/" + name);
		if (!f.exists() || forceOverwrite) {
			
			//Ensure we have an appropriate folder
			new File(path).mkdirs();
			_copy_asset(name, path + "/" + name);
		}
	}

	public void _copy_asset(String name_in, String name_out) {
		AssetManager assets = this.getAssets();

		try {
			InputStream in = assets.open(name_in);
			OutputStream out = new FileOutputStream(name_out);

			copy_stream(in, out);

			out.close();
			in.close();

		} catch (Exception e) {

			e.printStackTrace();
		}

	}

	public static void copy_stream(InputStream in, OutputStream out)
			throws IOException {
		byte[] buf = new byte[1024];
		while (true) {
			int count = in.read(buf);
			if (count <= 0)
				break;
			out.write(buf, 0, count);
		}
	}


	@Override protected void onStart()
	{
		Log.v( TAG, "GLES3JNIActivity::onStart()" );
		super.onStart();

		GLES3JNILib.onStart( mNativeHandle, this );
	}

	@Override protected void onResume()
	{
		Log.v( TAG, "GLES3JNIActivity::onResume()" );
		super.onResume();

		GLES3JNILib.onResume( mNativeHandle );
	}

	@Override protected void onPause()
	{
		Log.v( TAG, "GLES3JNIActivity::onPause()" );
		GLES3JNILib.onPause( mNativeHandle );
		super.onPause();
	}

	@Override protected void onStop()
	{
		Log.v( TAG, "GLES3JNIActivity::onStop()" );
		GLES3JNILib.onStop( mNativeHandle );
		super.onStop();
	}

	@Override protected void onDestroy()
	{
		Log.v( TAG, "GLES3JNIActivity::onDestroy()" );

		if ( mSurfaceHolder != null )
		{
			GLES3JNILib.onSurfaceDestroyed( mNativeHandle );
		}

		GLES3JNILib.onDestroy( mNativeHandle );

		super.onDestroy();
		mNativeHandle = 0;
	}

	@Override public void surfaceCreated( SurfaceHolder holder )
	{
		Log.v( TAG, "GLES3JNIActivity::surfaceCreated()" );
		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onSurfaceCreated( mNativeHandle, holder.getSurface() );
			mSurfaceHolder = holder;
		}
	}

	@Override public void surfaceChanged( SurfaceHolder holder, int format, int width, int height )
	{
		Log.v( TAG, "GLES3JNIActivity::surfaceChanged()" );
		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onSurfaceChanged( mNativeHandle, holder.getSurface() );
			mSurfaceHolder = holder;
		}
	}
	
	@Override public void surfaceDestroyed( SurfaceHolder holder )
	{
		Log.v( TAG, "GLES3JNIActivity::surfaceDestroyed()" );
		if ( mNativeHandle != 0 )
		{
			GLES3JNILib.onSurfaceDestroyed( mNativeHandle );
			mSurfaceHolder = null;
		}
	}
}
