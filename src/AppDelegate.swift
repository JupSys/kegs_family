// $KmKId: AppDelegate.swift,v 1.16 2021-01-24 05:36:13+00 kentd Exp $
// Copyright 2019-2020 by Kent Dickey
//
// /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/
//  Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/CoreAudio.framework/
//  Versions/A/Headers

import Cocoa

let Context_draw = false
	// Default: use safe draw function.
	// If true, use NSGraphicsContext.current.data to try to write
	//  directly to screen memory (in a different ARGB format)

class Window_info {
	var x_win : NSWindow? = nil
	var mac_view : MainView? = nil
	var kimage_ptr : UnsafeMutablePointer<Kimage>! = nil
	var title : String = ""
	var app_delegate : AppDelegate! = nil

//	init(_ new_is_main: Bool) {
//		is_main = new_is_main
//	}

	func set_kimage(_ kimage_ptr : UnsafeMutablePointer<Kimage>!,
				title: String, delegate: AppDelegate!) {
		self.kimage_ptr = kimage_ptr
		self.title = title
		self.app_delegate = delegate
	}

	func create_window() {
		let width = Int(video_get_a2_width(kimage_ptr))
		let height = Int(video_get_a2_height(kimage_ptr))
		let windowRect = NSRect(x: 100, y: 300, width: width,
							height: height)
		var window : NSWindow!
		var view : MainView!
		let style : NSWindow.StyleMask = [.titled, .closable,
								.resizable]

		window = NSWindow(contentRect: windowRect,
			styleMask: style,
			backing: .buffered, defer: false)

		let viewRect = NSRect(x: 0, y: 0, width: windowRect.size.width,
						height: windowRect.size.height)
		print("About to init MainView");
		view = MainView(frame: viewRect)
		print("About to set kimage_ptr");
		view.kimage_ptr = kimage_ptr;
		print("About to call initialize");
		view.initialize()
		view.closed = false 

		window.delegate = app_delegate
		window.contentView = view
		window.makeKeyAndOrderFront(app_delegate)
		window.acceptsMouseMovedEvents = true
		window.title = title
		window.showsToolbarButton = true
		window.contentAspectRatio = NSSize(width: width, height: height)
		video_set_active(kimage_ptr, Int32(1))
		video_update_scale(kimage_ptr, Int32(width), Int32(height))

		x_win = window
		mac_view = view
		window.makeKey()
	}

	func update() {
		// Decide if window should be opened/closed (if it's the
		//  debugger window), and call mac_update_display() to update
		let a2_active = video_get_active(kimage_ptr)
		if let view = mac_view {
			if(a2_active == 0 && !view.closed) {
				print("a2_active 0 on \(title), calling close")
				x_win!.orderOut(x_win)
				view.closed = true
			} else if(a2_active != 0 && view.closed) {
				print("Opening closed window \(title)")
				view.closed = false
				x_win!.orderFront(x_win)
				x_win!.makeKey()		// Move to front
			} else if(a2_active != 0) {
				view.mac_update_display()
			}
		} else {
			if(a2_active != 0) {
				print("Opening window \(title)")
				create_window()
			}
		}
	}

	func update_window_size(width: Int, height: Int) {
		//video_update_scale(kimage_ptr, Int32(width), Int32(height));
	}
}

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate, NSWindowDelegate {

	var mainwin_info = Window_info();
	var debugwin_info = Window_info();

	func find_win_info(_ window: NSWindow) -> Window_info {
		if(mainwin_info.x_win == window) {
			return mainwin_info
		}
		return debugwin_info
	}
	@objc func do_about(_:AnyObject) {
		print("About")
	}
	func applicationDidFinishLaunching(_ aNotification: Notification) {
		// This is your first real entry point into the app
		print("start!")
		set_menu_for_kegs()
		main_init()
	}

	func applicationWillTerminate(_ aNotification: Notification) {
		// Insert code here to tear down your application
	}

	func applicationShouldTerminateAfterLastWindowClosed(
				_ theApplication: NSApplication) -> Bool {
		// Application will close if main window is closed
		return true
	}
	func windowDidBecomeKey(_ notification: Notification) {
		//print("DidbecomeKey")
		// If window focus is changing, turn off key repeat
		adb_kbd_repeat_off()
	}
	func windowDidResignKey(_ notification: Notification) {
		//print("DidResignKey")
		adb_kbd_repeat_off()
	}

	func windowWillResize(_ window: NSWindow, to frameSize: NSSize)
								-> NSSize {
		// print("WILL RESIZE app \(frameSize)")
		let width = Int(frameSize.width)
		let height = Int(frameSize.width)
		let win_info = find_win_info(window)
		win_info.update_window_size(width: width, height: height)
		return frameSize
	}
	func windowShouldClose(_ window: NSWindow) -> Bool {
		print("windowShouldClose")
		let win_info = find_win_info(window)
		if(mainwin_info.x_win == window) {
			// User has clicked the close box on the main emulator
			//  window.  Just exit the app
			NSApp.terminate(window)
			return true		// Let main window close
		} else {
			video_set_active(win_info.kimage_ptr, Int32(0))
			win_info.update()
			return false
		}
	}

	func set_menu_for_kegs() {
		let appname = "Kegs"
		if let menu = NSApp.mainMenu {
			show_menu(menu, depth: 0)
			menu.removeAllItems()

			print("Installing my menu now")
			let kegs = NSMenu(title: appname)
			kegs.addItem(withTitle: "About \(appname)",
				action: #selector(AppDelegate.do_about(_:)),
				keyEquivalent: "")
			kegs.addItem(NSMenuItem.separator())
			kegs.addItem(withTitle: "Quit \(appname)",
				action: #selector(NSApplication.terminate(_:)),
				keyEquivalent: "Q")
			let kegs_item = NSMenuItem()
			kegs_item.title = appname
			kegs_item.submenu = kegs
			menu.addItem(kegs_item)

			// First menu of "Kegs" now done.  Add "Edit" menu
			let edit = NSMenu(title: "Edit")
			edit.addItem(withTitle: "Paste",
				action: #selector(MainView.do_paste(_:)),
				keyEquivalent: "")
			let edit_item = NSMenuItem()
			edit_item.title = "Edit"
			edit_item.submenu = edit
			menu.addItem(edit_item)

			// Edit menu of "Kegs" now done.  Add "Config" menu
			let config = NSMenu(title: "Config")
			config.addItem(withTitle: "Configuration  F4",
				action: #selector(MainView.do_config(_:)),
				keyEquivalent: "")
			let config_item = NSMenuItem()
			config_item.title = "Config"
			config_item.submenu = config
			menu.addItem(config_item)
		
			show_menu(menu, depth: 0)
		}
	}

	func show_menu(_ menu: NSMenu, depth: Int) {
		if(depth >= -10) {
			return		// HACK: remove to see debug output!
		}
		print("menu at depth \(depth): \(menu.title)")
		if(depth > 5) {		// Prevent infinit recursion
			return
		}
		for menuit in menu.items {
			print("menuit: depth:\(depth) \(menuit.title)")
			print("  keyeq:\(menuit.keyEquivalent)")
			print("  modifiers:\(menuit.keyEquivalentModifierMask)")
			print("  isSeparator:\(menuit.isSeparatorItem)")
			if let sub = menuit.submenu {
				show_menu(sub, depth: depth+1)
			}
		}
	}

	var mainWindow : NSWindow!
	var mainwin_view : MainView!
    
	func main_init() {
		var kimage_ptr : UnsafeMutablePointer<Kimage>!

		let argc = CommandLine.argc
		let argv = CommandLine.unsafeArgv
		parse_argv(argc, argv, 3);

		kegs_init(24)
		if(Context_draw) {
			video_set_blue_mask(UInt32(0x0000ff))
			video_set_green_mask(UInt32(0x00ff00))
			video_set_red_mask(UInt32(0xff0000))
		} else {
			video_set_red_mask(UInt32(0x0000ff))
			video_set_green_mask(UInt32(0x00ff00))
			video_set_blue_mask(UInt32(0xff0000))
		}
		video_set_palette()
		kimage_ptr = video_get_kimage(Int32(0))
		mainwin_info.set_kimage(kimage_ptr, title: "KEGS",
							delegate: self)
		kimage_ptr = video_get_kimage(Int32(1))
		debugwin_info.set_kimage(kimage_ptr, title: "Debugger",
							delegate: self)

		mainwin_info.create_window()
		main_run_loop()
	}

	func main_run_loop() {
		DispatchQueue.main.asyncAfter(deadline: .now() +
							.milliseconds(1)) {
			self.main_run_loop()
		}
		run_16ms()
		mainwin_info.update()
		debugwin_info.update()
	}
}

