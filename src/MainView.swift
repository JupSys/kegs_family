// $KmKId: MainView.swift,v 1.22 2021-01-10 20:42:42+00 kentd Exp $
//  Copyright 2019-2020 by Kent Dickey
//

import Cocoa
import CoreGraphics
import AudioToolbox

class MainView: NSView {

	var bitmapContext : CGContext!
	var bitmapData : UnsafeMutableRawPointer!
	var rawData : UnsafeMutablePointer<UInt32>!
	var current_flags : UInt = 0
	var mouse_moved : Bool = false
	var mac_warp_pointer : Int32 = 0
	var mac_hide_pointer : Int32 = 0
	var kimage_ptr : UnsafeMutablePointer<Kimage>!
	var closed : Bool = false
	var pixels_per_line = 640
	var max_height = 600

	let is_cmd = UInt(NSEvent.ModifierFlags.command.rawValue)
	let is_control = UInt(NSEvent.ModifierFlags.control.rawValue)
	let is_shift = NSEvent.ModifierFlags.shift.rawValue
	let is_capslock = NSEvent.ModifierFlags.capsLock.rawValue
	let is_option = NSEvent.ModifierFlags.option.rawValue


	override init(frame frameRect: NSRect) {
		super.init(frame: frameRect)
	}

	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}

	func windowDidResize(_ notification: Notification) {
		print("DID RESIZE")
	}
	override func performKeyEquivalent(with event: NSEvent) -> Bool {
		let keycode = event.keyCode
		let is_repeat = event.isARepeat
		// print(".performKeyEquiv keycode: \(keycode), is_repeat: " +
		//					"\(is_repeat)")
		if(((current_flags & is_cmd) == 0) || is_repeat) {
			// If CMD isn't being held down, just ignore this
			return false
		}
		// Otherwise, manually do down, then up, for this key
		adb_physical_key_update(kimage_ptr, Int32(keycode), 0,
			Int32(current_flags & is_shift),
			Int32(current_flags & is_control),
			Int32(current_flags & is_capslock))
		adb_physical_key_update(kimage_ptr, Int32(keycode), 1,
			Int32(current_flags & is_shift),
			Int32(current_flags & is_control),
			Int32(current_flags & is_capslock))
		return true
	}

	override var acceptsFirstResponder: Bool {
		return true
	}
	override func keyDown(with event: NSEvent) {
		let keycode = event.keyCode
		let is_repeat = event.isARepeat;
		// print(".keyDown code: \(keycode), repeat: \(is_repeat)")
		if(is_repeat) {
			// If we do CMD-E, then we never get a down for the E,
			//  but we will get repeat events for that E.  Let's
			//  ignore those
			return
		}
		adb_physical_key_update(kimage_ptr, Int32(keycode), 0,
			Int32(current_flags & is_shift),
			Int32(current_flags & is_control),
			Int32(current_flags & is_capslock))
	}

	override func keyUp(with event: NSEvent) {
		let keycode = event.keyCode
		// let is_repeat = event.isARepeat;
		// print(".keyUp code: \(keycode), repeat: \(is_repeat)")
		adb_physical_key_update(kimage_ptr, Int32(keycode), 1,
			Int32(current_flags & is_shift),
			Int32(current_flags & is_control),
			Int32(current_flags & is_capslock))
	}

	override func flagsChanged(with event: NSEvent) {
		let flags = event.modifierFlags.rawValue &
				(is_cmd | is_control | is_shift | is_capslock |
								is_option)
		let flags_xor = current_flags ^ flags
		let shift_down = Int32(flags & is_shift)
		let control_down = Int32(flags & is_control)
		let caps_down = Int32(flags & is_capslock)
		if((flags_xor & is_control) != 0) {
			let is_up = ((flags & is_control) == 0) ? 1 : 0
			adb_physical_key_update(kimage_ptr, 0x36, Int32(is_up),
				shift_down, control_down, caps_down)
			// print("control 0x35, is_up:\(is_up)")
		}
		if((flags_xor & is_capslock) != 0) {
			let is_up = ((flags & is_capslock) == 0) ? 1 : 0
			adb_physical_key_update(kimage_ptr, 0x39, Int32(is_up),
				shift_down, control_down, caps_down)
			// print("capslock 0x39, is_up:\(is_up)")
		}
		if((flags_xor & is_shift) != 0) {
			let is_up = ((flags & is_shift) == 0) ? 1 : 0
			adb_physical_key_update(kimage_ptr, 0x38, Int32(is_up),
				shift_down, control_down, caps_down)
			// print("shift 0x38, is_up:\(is_up)")
		}
		if((flags_xor & is_cmd) != 0) {
			let is_up = ((flags & is_cmd) == 0) ? 1 : 0
			adb_physical_key_update(kimage_ptr, 0x37, Int32(is_up),
				shift_down, control_down, caps_down)
			// print("command 0x37, is_up:\(is_up)")
		}
		if((flags_xor & is_option) != 0) {
			let is_up = ((flags & is_option) == 0) ? 1 : 0
			adb_physical_key_update(kimage_ptr, 0x3a, Int32(is_up),
				shift_down, control_down, caps_down)
			// print("option 0x3a, is_up:\(is_up)")
		}
		current_flags = flags
		// print("flagsChanged: \(flags) and keycode: \(event.keyCode)")
	}
	override func acceptsFirstMouse(for event: NSEvent?) -> Bool {
		// This is to let the first click when changing to this window
		//  through to the app, I probably don't want this.
		return false
	}
	override func mouseMoved(with event: NSEvent) {
		//let type = event.eventNumber
		//print(" event type: \(type)")
		mac_update_mouse(event: event, buttons_state:0, buttons_valid:0)

	}
	override func mouseDragged(with event: NSEvent) {
		mac_update_mouse(event: event, buttons_state: 0,
							buttons_valid: 0)
	}
	override func otherMouseDown(with event: NSEvent) {
		mac_update_mouse(event: event, buttons_state:2, buttons_valid:2)
	}
	override func otherMouseUp(with event: NSEvent) {
		mac_update_mouse(event: event, buttons_state:0, buttons_valid:2)
	}

	override func mouseEntered(with event: NSEvent) {
		print("mouse entered")
	}
	override func rightMouseUp(with event: NSEvent) {
		mac_update_mouse(event: event, buttons_state:0, buttons_valid:4)
	}
	override func rightMouseDown(with event: NSEvent) {
		print("Right mouse down")
		mac_update_mouse(event: event, buttons_state:4, buttons_valid:4)
	}
	override func mouseUp(with event: NSEvent) {
		// print("Mouse up \(event.locationInWindow.x)," +
		//				"\(event.locationInWindow.y)")
		mac_update_mouse(event: event, buttons_state:0, buttons_valid:1)
	}
	override func mouseDown(with event: NSEvent) {
		// print("Mouse down \(event.locationInWindow.x)," +
		//				"\(event.locationInWindow.y)")
		mac_update_mouse(event: event, buttons_state:1, buttons_valid:1)
	}

	func mac_update_mouse(event: NSEvent, buttons_state: Int,
							buttons_valid: Int) {
		var warp = Int32(0)
		var x_width = 0
		var y_height = 0
		let hide = adb_get_hide_warp_info(&warp)
		if(warp != mac_warp_pointer) {
			mouse_moved = true
		}
		mac_warp_pointer = warp
		if(mac_hide_pointer != hide) {
			mac_hide_pointer(hide: hide)
		}
		mac_hide_pointer = hide
		let location = event.locationInWindow
		if(!Context_draw) {
			// We're letting the Mac scale the window for us,
			//  so video_scale_mouse*() doesn't know the scale
			//  factor, so pass it in
			x_width = Int(bounds.size.width);
			y_height = Int(bounds.size.height);
		}
		let x = video_scale_mouse_x(kimage_ptr, Int32(location.x),
						Int32(x_width))
		let y = video_scale_mouse_y(kimage_ptr,
				Int32(bounds.size.height - 1 - location.y),
				Int32(y_height))
		if((mac_warp_pointer != 0) && (x == A2_WINDOW_WIDTH/2) &&
					(y == A2_WINDOW_HEIGHT/2) &&
					(buttons_valid == 0)) {
			update_mouse(kimage_ptr, x, y, Int32(0), Int32(-1))
			return
		}
		let ret = update_mouse(kimage_ptr, x, y,
				Int32(buttons_state), Int32(buttons_valid))
		if(ret != 0) {
			mouse_moved = true
		}
	}

	func mac_hide_pointer(hide: Int32) {
		print("Hide called: \(hide)")
		if(hide != 0) {
			CGDisplayHideCursor(CGMainDisplayID())
		} else {
			CGDisplayShowCursor(CGMainDisplayID())
		}
	}
	func initialize() {
		//let colorSpace = CGColorSpace(name: CGColorSpace.sRGB)
		print("Initialize view called")
		let width = Int(frame.size.width)
		let height = Int(frame.size.height)
		//if let screen = NSScreen.main {
		//	let rect = screen.frame
		//	width = Int(rect.size.width)
		//	height = Int(rect.size.height)
		//}
		pixels_per_line = width
		max_height = height
		print("pixels_per_line: \(pixels_per_line), " +
				"max_height: \(max_height)")

		let color_space = CGDisplayCopyColorSpace(CGMainDisplayID())
		//let colorSpace = CGColorSpaceCreateDeviceRGB()
		bitmapContext = CGContext(data: nil, width: width,
			height: height, bitsPerComponent: 8,
			bytesPerRow: width * 4,
			space: color_space,
			bitmapInfo: CGImageAlphaInfo.noneSkipLast.rawValue)

		//CGImageAlphaInfo.noneSkipLast.rawValue
		bitmapData = bitmapContext.data!
		// Set the intial value of the data to black (0)
		bitmapData.initializeMemory(as: UInt32.self, repeating: 0,
						count: height * width)
		rawData = bitmapData.bindMemory(to: UInt32.self,
						capacity: height * width)
		video_update_scale(kimage_ptr, Int32(width), Int32(height))
		if(Context_draw) {
			video_set_alpha_mask(UInt32(0xff000000))
			// Set video.c alpha mask, since 0 means transparent
		}
	}

	override func draw(_ dirtyRect: NSRect) {
		var rect : Change_rect
		//super.draw(dirtyRect)
			// Documentation says super.draw not needed...
		// Draw the current image buffer to the screen
		let context = NSGraphicsContext.current!.cgContext
		if(!Context_draw) {
			// Safe, simple drawing
			let image = bitmapContext.makeImage()!
			context.draw(image, in: bounds)
				// The above call does the actual copy of
				//  data to the screen, and can take a while
			//print("Draw, bounds:\(bounds), dirtyr:\(dirtyRect)")
		} else {
			// Unsafe, more direct drawing by peeking into
			// NSGraphicsContext.current.data
			if let data = context.data {
				rect = Change_rect(x:0, y:0,
					width:Int32(context.width),
					height:Int32(context.height));
				video_update_scale(kimage_ptr,
					Int32(context.width),
					Int32(context.height))
				video_out_data_scaled(data, kimage_ptr,
					Int32(context.bytesPerRow/4), &rect);
				video_out_done(kimage_ptr);
				print("Did out_data_scaled, rect:\(rect)");
			}
		}
	}

	@objc func do_config(_ : AnyObject) {
		// Create a "virtual" F4 press
		//print("do_config")
		// Create a keydown for the F4 key (keycode:0x76)
		adb_physical_key_update(kimage_ptr, Int32(0x76), 0,
			Int32(current_flags & is_shift),
			Int32(current_flags & is_control),
			Int32(current_flags & is_capslock))

		// and create a keyup for the F4 key (keycode:0x76)
		adb_physical_key_update(kimage_ptr, Int32(0x76), 1,
			Int32(current_flags & is_shift),
			Int32(current_flags & is_control),
			Int32(current_flags & is_capslock))
	}

	@objc func do_paste(_ : AnyObject) {
		// print("do_paste")
		let general = NSPasteboard.general;
		guard let str = general.string(forType: .string) else {
			print("Cannot paste, nothing in clipboard");
			return
		}
		//print("str: \(str)")
		for raw_c in str.utf8 {
			var c: Int
			c = Int(raw_c)
			// Don't paste in ctrl-chars and other junk
			if(c == 10) {
				c = 13		// Newline to Return
			} else if((c == 9) || (c == 13)) {	// Tab, return
				// Just allow it
			} else if(c < 32) {
				c = 0
			}
			if((c != 0) && (c < 0x7f)) {
				let ret = adb_paste_add_buf(UInt32(c))
				if(ret != 0) {
					print("Paste too large!")
					return;
				}
			}
		}
	}

	func mac_update_display() {
		var valid : Int32
		var rect : Change_rect
		var dirty_rect = NSRect.zero

		if(Context_draw) {
			// We just need to know if there are any changes,
			//  don't actually do the copies now
			valid = video_out_query(kimage_ptr);
			if(valid != 0) {
				self.setNeedsDisplay(bounds)
				print("Needs display");
			}
			return;
		}
		// Otherwise, update rawData in our Bitmap now
		rect = Change_rect(x:0, y:0, width:0, height:0)
		for i in 0...MAX_CHANGE_RECTS {		// MAX_CHANGE_RECTS
			valid = video_out_data(rawData, kimage_ptr,
				Int32(pixels_per_line), &rect, Int32(i))
			if(valid == 0) {
				break
			}
			dirty_rect.origin.x = CGFloat(rect.x)
			dirty_rect.origin.y = bounds.size.height -
					CGFloat(rect.y) - CGFloat(rect.height)
			dirty_rect.size.width = CGFloat(rect.width)
			dirty_rect.size.height = CGFloat(rect.height)

			self.setNeedsDisplay(bounds)
				// It's necessary to redraw the whole screen,
				//  there's no mechanism to just redraw part
				// The coordinates would need transformation
				//  (since Mac 0,0 is the lower left corner)
			//self.setNeedsDisplay(dirty_rect)
		}
	}
}
