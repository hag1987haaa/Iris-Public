module.exports = [
  {
    "type": "heading",
    "defaultValue": "Iris Settings"
  },
  {
    "type": "text",
    "defaultValue": "Customize your mechanical shutter watchface."
  },
  
  // ==========================================
  // 1. 見た目の設定 (Appearance)
  // ==========================================
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Appearance"
      },
      {
        "type": "select",
        "messageKey": "WatchFaceMode",
        "label": "Appearance Mode",
        "defaultValue": "0",
        "options": [
          { "label": "Digital", "value": "0" },
          { "label": "Analog", "value": "1" }
        ]
      },
			      {
        "type": "select",
        "messageKey": "ShutterThickness",
        "label": "Aperture Size",
        "defaultValue": "1",
        "options": [
          { "label": "Wide", "value": "0" },
          { "label": "Medium", "value": "1" },
          { "label": "Narrow", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "Theme",
        "label": "Background Style",
        "defaultValue": "0",
        "capabilities": ["COLOR"],
        "options": [
          { "label": "Solid White", "value": "0" },
          { "label": "Solid Black", "value": "1" },
          { "label": "Custom Photo (Image)", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "Theme",
        "label": "Background Style",
        "defaultValue": "0",
        "capabilities": ["BW"],
        "options": [
          { "label": "Solid White", "value": "0" },
          { "label": "Solid Black", "value": "1" }
        ]
      },
      {
        "type": "select",
        "messageKey": "TextColor",
        "label": "Text / Hand Color",
        "defaultValue": "0",
        "options": [
          { "label": "Auto (Invert Background)", "value": "0" },
          { "label": "Black", "value": "1" },
          { "label": "White", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "ShutterColorMode",
        "label": "Blade Color Style",
        "defaultValue": "4", 
        "capabilities": ["COLOR"],
        "options": [
          { "label": "Solid Black", "value": "0" },
          { "label": "Solid White", "value": "1" },
          { "label": "Custom Solid Color", "value": "2" },
          { "label": "Vivid Rainbow", "value": "3" },
          { "label": "Pastel Rainbow", "value": "4" },
          { "label": "Battery Sync Vivid", "value": "5" },
          { "label": "Battery Sync Pastel", "value": "6" }
        ]
      },
      {
        "type": "select",
        "messageKey": "ShutterColorMode",
        "label": "Blade Color Style",
        "defaultValue": "0", 
        "capabilities": ["BW"], 
        "options": [
          { "label": "Solid Black", "value": "0" },
          { "label": "Solid White", "value": "1" }
        ]
      },
      {
        "type": "color",
        "messageKey": "CustomColor",
        "label": "Custom Blade Color",
        "description": "Used only if 'Custom Solid Color' is selected above.",
        "defaultValue": "00FFFF",
        "allowFading": false,
        "capabilities": ["COLOR"]
      },
      {
        "type": "toggle",
        "messageKey": "BatteryColorSync",
        "label": "Drop Blades on Low Battery",
        "description": "Blades will drop when battery is under 50%.",
        "defaultValue": true
      },
      {
        "type": "select",
        "messageKey": "TextStyle",
        "label": "Text Style (Effect)",
        "description": "Add drop shadow or outline to time and info text for high contrast over photos.",
        "defaultValue": "0",
        "options": [
          { "label": "Flat (None)", "value": "0" },
          { "label": "Drop Shadow", "value": "1" },
          { "label": "Outline", "value": "2" }
        ]
      }
    ]	
  },

  // ==========================================
  // 2. カスタム背景画像 (Custom Background Image)
  // ==========================================
  {
    "type": "section",
    "capabilities": ["COLOR"],
    "items": [
      {
        "type": "heading",
        "defaultValue": "📸 Custom Background Image"
      },
      {
        "type": "text",
        "id": "bg_normal_container",
        "defaultValue": "<div id='bg_normal_container'></div>"
      }
    ]
  },

  // ==========================================
  // 3. 動きと振動 (Animation & Vibration)
  // ==========================================
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Animation & Vibration"
      },
      {
        "type": "select",
        "messageKey": "AnimMode",
        "label": "Animation Mode",
        "description": "Choose when aperture animation plays.",
        "defaultValue": "0",
        "options": [
          { "label": "Full (Shake & Timer)", "value": "0" },
          { "label": "Interval Timer Only", "value": "1" },
          { "label": "Shake Only", "value": "2" },
          { "label": "Disabled (Off)", "value": "3" }
        ]
      },
      {
        "type": "select",
        "messageKey": "AnimFreq",
        "label": "Auto-Animation Interval",
        "description": "How often the aperture animation plays automatically.",
        "defaultValue": "1",
        "options": [
          { "label": "Every 1 Minute", "value": "1" },
          { "label": "Every 5 Minutes", "value": "5" },
          { "label": "Every 10 Minutes", "value": "10" },
          { "label": "Every 15 Minutes", "value": "15" },
          { "label": "Every 20 Minutes", "value": "20" },
          { "label": "Every 30 Minutes", "value": "30" },
          { "label": "Every 1 Hour", "value": "60" }
        ]
      },
      {
        "type": "select",
        "messageKey": "AnimSpeed",
        "label": "Animation Speed",
        "description": "Choose blade transition speed.",
        "defaultValue": "40",
        "options": [
          { "label": "Fast (200ms)", "value": "20" },
          { "label": "Normal (400ms)", "value": "40" },
          { "label": "Smooth (600ms)", "value": "60" },
          { "label": "Cinematic (800ms)", "value": "80" },
          { "label": "Slow (1000ms)", "value": "100" }
        ]
      },
      {
        "type": "text",
        "defaultValue": "<b>Vibration & Haptics</b>"
      },
      {
        "type": "select",
        "messageKey": "VibeMode",
        "label": "Vibration Mode",
        "description": "Choose vibration feedback. Pauses automatically during Sleep Mode / Quiet Time outside business hours.",
        "defaultValue": "2",
        "options": [
          { "label": "Shake & Periodic Vibration", "value": "0" },
          { "label": "Periodic Vibration Only", "value": "1" },
          { "label": "Shake Only", "value": "2" },
          { "label": "Disabled (Off)", "value": "3" }
        ]
      },
      {
        "type": "select",
        "messageKey": "ChimeInterval",
        "label": "Periodic Vibration Interval",
        "description": "How often the watch gently vibrates.",
        "defaultValue": "60",
        "options": [
          { "label": "Every 5 Minutes", "value": "5" },
          { "label": "Every 10 Minutes", "value": "10" },
          { "label": "Every 15 Minutes", "value": "15" },
          { "label": "Every 20 Minutes", "value": "20" },
          { "label": "Every 30 Minutes", "value": "30" },
          { "label": "Every 1 Hour", "value": "60" }
        ]
      }
    ]
  },
	
	{
    "type": "submit",
    "defaultValue": "SAVE SETTINGS"
	  },
	
  // ==========================================
  // 3. ビジネスタイム (Business Time)
  // ==========================================
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "🏢 Business Time"
      },
{
        "type": "toggle",
        "messageKey": "EnableBusiness",
        "label": "Enable Business Time", // 💡 スッキリ！
        "defaultValue": false,
        "description": "When Quiet Time is active during your set hours, the watch automatically switches to a professional look, pauses regular animations, and uses business vibration settings. Outside these hours, it enters a muted Sleep Mode." // 💡 Chimeを消去＆UX向上！
      },
			
      {
        "type": "select",
        "messageKey": "BizDays",
        "label": "Active Days",
        "defaultValue": "1",
        "options": [
          { "label": "Everyday", "value": "0" },
          { "label": "Weekdays (Mon-Fri)", "value": "1" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizStartH",
        "defaultValue": "9",
        "label": "Business Start Time",
        "description": "Set Start and End to the same time for 24-hour business mode.",
        "options": [
          { "label": "0:00 (Midnight)", "value": "0" },
          { "label": "1:00", "value": "1" },
          { "label": "2:00", "value": "2" },
          { "label": "3:00", "value": "3" },
          { "label": "4:00", "value": "4" },
          { "label": "5:00", "value": "5" },
          { "label": "6:00", "value": "6" },
          { "label": "7:00", "value": "7" },
          { "label": "8:00", "value": "8" },
          { "label": "9:00", "value": "9" },
          { "label": "10:00", "value": "10" },
          { "label": "11:00", "value": "11" },
          { "label": "12:00 (Noon)", "value": "12" },
          { "label": "13:00 (1 PM)", "value": "13" },
          { "label": "14:00 (2 PM)", "value": "14" },
          { "label": "15:00 (3 PM)", "value": "15" },
          { "label": "16:00 (4 PM)", "value": "16" },
          { "label": "17:00 (5 PM)", "value": "17" },
          { "label": "18:00 (6 PM)", "value": "18" },
          { "label": "19:00 (7 PM)", "value": "19" },
          { "label": "20:00 (8 PM)", "value": "20" },
          { "label": "21:00 (9 PM)", "value": "21" },
          { "label": "22:00 (10 PM)", "value": "22" },
          { "label": "23:00 (11 PM)", "value": "23" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizEndH",
        "defaultValue": "18",
        "label": "Business End Time",
        "description": "Set End Time smaller than Start Time (e.g. 22 to 2) for overnight shifts.",
        "options": [
          { "label": "0:00 (Midnight)", "value": "0" },
          { "label": "1:00", "value": "1" },
          { "label": "2:00", "value": "2" },
          { "label": "3:00", "value": "3" },
          { "label": "4:00", "value": "4" },
          { "label": "5:00", "value": "5" },
          { "label": "6:00", "value": "6" },
          { "label": "7:00", "value": "7" },
          { "label": "8:00", "value": "8" },
          { "label": "9:00", "value": "9" },
          { "label": "10:00", "value": "10" },
          { "label": "11:00", "value": "11" },
          { "label": "12:00 (Noon)", "value": "12" },
          { "label": "13:00 (1 PM)", "value": "13" },
          { "label": "14:00 (2 PM)", "value": "14" },
          { "label": "15:00 (3 PM)", "value": "15" },
          { "label": "16:00 (4 PM)", "value": "16" },
          { "label": "17:00 (5 PM)", "value": "17" },
          { "label": "18:00 (6 PM)", "value": "18" },
          { "label": "19:00 (7 PM)", "value": "19" },
          { "label": "20:00 (8 PM)", "value": "20" },
          { "label": "21:00 (9 PM)", "value": "21" },
          { "label": "22:00 (10 PM)", "value": "22" },
          { "label": "23:00 (11 PM)", "value": "23" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizFace",
        "label": "Appearance Mode",
        "defaultValue": "1",
        "options": [
          { "label": "Digital", "value": "0" },
          { "label": "Analog", "value": "1" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizShutter",
        "label": "Aperture Size",
        "defaultValue": "0",
        "options": [
          { "label": "Wide", "value": "0" },
          { "label": "Medium", "value": "1" },
          { "label": "Narrow", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizBgMode",
        "label": "Background Style",
        "defaultValue": "0",
        "capabilities": ["COLOR"],
        "options": [
          { "label": "Solid Black", "value": "0" },
          { "label": "Solid White", "value": "1" },
          { "label": "Same as Normal Background", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizBgMode",
        "label": "Background Style",
        "defaultValue": "0",
        "capabilities": ["BW"],
        "options": [
          { "label": "Solid Black", "value": "0" },
          { "label": "Solid White", "value": "1" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizBladeColor",
        "label": "Blade Color Style",
        "defaultValue": "0",
        "options": [
          { "label": "Solid Black", "value": "0" },
          { "label": "Solid White", "value": "1" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizTextColor",
        "label": "Text / Hand Color",
        "defaultValue": "0",
        "options": [
          { "label": "Auto (Invert Background)", "value": "0" },
          { "label": "Black", "value": "1" },
          { "label": "White", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizVibeMode",
        "label": "Business Vibration Mode",
        "description": "Wrist shake is disabled during business hours to maintain a quiet, professional demeanor.",
        "defaultValue": "1",
        "options": [
          { "label": "Periodic Vibration Only", "value": "1" },
          { "label": "Disabled (Off / Mute)", "value": "3" }
        ]
      },
      {
        "type": "select",
        "messageKey": "BizChimeInterval",
        "label": "Business Vibration Interval",
        "description": "How often the watch gently vibrates during business hours.",
        "defaultValue": "60",
        "options": [
          { "label": "Every 5 Minutes", "value": "5" },
          { "label": "Every 10 Minutes", "value": "10" },
          { "label": "Every 15 Minutes", "value": "15" },
          { "label": "Every 20 Minutes", "value": "20" },
          { "label": "Every 30 Minutes", "value": "30" },
          { "label": "Every 1 Hour", "value": "60" }
        ]
      }
    ]
  },

  
  // ==========================================
  // 4. サポート (Support)
  // ==========================================
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "☕️ Support the Developer"
      },
      {
        "type": "text",
        "defaultValue": "If you enjoy Iris, consider buying me a coffee to keep the gears turning!"
      },
      {
        "type": "text",
        "defaultValue": "<a href='https://ko-fi.com/1987haaa' target='_blank'>☕️ Tap here to buy me a coffee on Ko-fi!</a>"
			}
    ]
  },
  {
    "type": "submit",
    "defaultValue": "SAVE SETTINGS"
  }
];