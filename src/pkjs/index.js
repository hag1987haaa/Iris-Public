var Clay = require('@rebble/clay');
var clayConfig = require('./Config.js');
var customClay = require('./custom-clay.js');
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });

function base64ToBytes(base64) {
  var binary = atob(base64);
  var len = binary.length;
  var bytes = [];
  for (var i = 0; i < len; i++) {
    bytes.push(binary.charCodeAt(i));
  }
  return bytes;
}

var isImageTransferring = false;

function sendImageQueue(jobs, onComplete) {
  if (!jobs || jobs.length === 0) {
    isImageTransferring = false;
    if (onComplete) onComplete();
    return;
  }
  isImageTransferring = true;
  var job = jobs.shift();

  if (job.action === 'clear') {
    var clearRetries = 0;
    function sendClear() {
      console.log('Sending clear command for target:', job.target);
      Pebble.sendAppMessage({ 'BgImageClear': job.target }, function() {
        sendImageQueue(jobs, onComplete);
      }, function(err) {
        clearRetries++;
        if (clearRetries > 5) {
          console.log('Max retries reached for clear command, aborting job.');
          sendImageQueue(jobs, onComplete);
          return;
        }
        console.log('Failed to send clear, retrying (' + clearRetries + ')...', JSON.stringify(err));
        setTimeout(sendClear, 300);
      });
    }
    sendClear();
    return;
  }

  var byteArray = job.byteArray;
  var CHUNK_SIZE = 2048;
  var totalChunks = Math.ceil(byteArray.length / CHUNK_SIZE);
  var currentChunk = 0;
  var chunkRetries = 0;

  console.log('Starting image transfer for target:', job.target, 'Total bytes:', byteArray.length, 'Total chunks:', totalChunks);

  function sendNextChunk() {
    if (currentChunk >= totalChunks) {
      console.log('Image transfer complete for target:', job.target);
      sendImageQueue(jobs, onComplete);
      return;
    }

    var start = currentChunk * CHUNK_SIZE;
    var end = Math.min(start + CHUNK_SIZE, byteArray.length);
    var chunkSlice = byteArray.slice(start, end);

    var message = {
      'BgImageData': chunkSlice,
      'BgImageTarget': job.target,
      'BgImageChunkIdx': currentChunk,
      'BgImageTotalChunks': totalChunks,
      'BgImageWidth': job.w,
      'BgImageHeight': job.h
    };

    Pebble.sendAppMessage(message, function() {
      currentChunk++;
      chunkRetries = 0;
      setTimeout(sendNextChunk, 10);
    }, function(err) {
      chunkRetries++;
      if (chunkRetries > 8) {
        console.log('Max retries exceeded at chunk ' + currentChunk + ', aborting image transfer.');
        isImageTransferring = false;
        return;
      }
      console.log('Error sending chunk ' + currentChunk + ', retrying (' + chunkRetries + ')...', JSON.stringify(err));
      setTimeout(sendNextChunk, 150);
    });
  }

  sendNextChunk();
}

function sendSavedBgTarget(target) {
  if (isImageTransferring) {
    console.log('Image transfer already in progress, skipping duplicate request for target:', target);
    return;
  }
  var key = (target === 0) ? 'iris_bg_normal_raw' : 'iris_bg_biz_raw';
  var raw = localStorage.getItem(key);
  if (!raw) return;
  try {
    var data = JSON.parse(raw);
    if (data && data.b64) {
      var bytes = base64ToBytes(data.b64);
      sendImageQueue([{
        action: 'send',
        target: target,
        w: data.w,
        h: data.h,
        byteArray: bytes
      }]);
    }
  } catch (err) {
    console.log('Error sending saved bg target:', err);
  }
}

Pebble.addEventListener('showConfiguration', function() {
  if (Pebble.getActiveWatchInfo) {
    clay.meta.activeWatchInfo = Pebble.getActiveWatchInfo();
  }
  clay.meta.accountToken = Pebble.getAccountToken();
  clay.meta.watchToken = Pebble.getWatchToken();

  var normalDraft = null;
  try {
    var nd = localStorage.getItem('iris_bg_normal_draft');
    if (nd) normalDraft = JSON.parse(nd);
  } catch (err) {
    console.log('Error reading drafts from localStorage:', err);
  }

  clay.meta.userData = {
    normalDraft: normalDraft
  };

  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) return;

  var responseStr = e.response.match(/^\{/) ? e.response : decodeURIComponent(e.response);
  var rawSettings = {};
  try {
    rawSettings = JSON.parse(responseStr);
  } catch (err) {
    console.log('Error parsing Clay response:', err);
    return;
  }

  var normalImg = null;

  if (rawSettings.iris_bg_normal) {
    normalImg = rawSettings.iris_bg_normal.value || rawSettings.iris_bg_normal;
    delete rawSettings.iris_bg_normal;
  }
  if (rawSettings.iris_bg_biz) {
    delete rawSettings.iris_bg_biz;
  }

  // ドラフト（次回設定画面オープン時の復元用メタデータ）の保存
  if (rawSettings.iris_bg_normal_draft !== undefined) {
    if (rawSettings.iris_bg_normal_draft === null) {
      localStorage.removeItem('iris_bg_normal_draft');
    } else {
      localStorage.setItem('iris_bg_normal_draft', JSON.stringify(rawSettings.iris_bg_normal_draft));
    }
    delete rawSettings.iris_bg_normal_draft;
  }
  if (rawSettings.iris_bg_biz_draft !== undefined) {
    delete rawSettings.iris_bg_biz_draft;
  }

  var jobs = [];

  if (normalImg) {
    if (normalImg.clear) {
      localStorage.removeItem('iris_bg_normal_raw');
      jobs.push({ action: 'clear', target: 0 });
    } else if (normalImg.b64) {
      localStorage.setItem('iris_bg_normal_raw', JSON.stringify(normalImg));
      jobs.push({
        action: 'send',
        target: 0,
        w: normalImg.w,
        h: normalImg.h,
        byteArray: base64ToBytes(normalImg.b64)
      });
    }
  }

  // 残りの標準設定を Clay 経由で時計へ送信＆localStorage保存
  var appMessageDict = clay.getSettings(JSON.stringify(rawSettings));
  Pebble.sendAppMessage(appMessageDict, function() {
    console.log('Sent Clay config to Pebble successfully');
    if (jobs.length > 0) {
      setTimeout(function() {
        sendImageQueue(jobs, function() {
          console.log('All image jobs completed successfully!');
        });
      }, 200);
    }
  }, function(err) {
    console.log('Failed to send Clay config:', JSON.stringify(err));
    if (jobs.length > 0) {
      setTimeout(function() {
        sendImageQueue(jobs, function() {
          console.log('All image jobs completed successfully!');
        });
      }, 500);
    }
  });
});

function fetchWeather() {
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      var lat = pos.coords.latitude;
      var lon = pos.coords.longitude;
      var url = 'https://api.open-meteo.com/v1/forecast?latitude=' + lat + '&longitude=' + lon + '&current_weather=true&daily=weathercode&timezone=auto';

      var req = new XMLHttpRequest();
      req.open('GET', url, true);
      req.onload = function() {
        if (req.readyState === 4 && req.status === 200) {
          var response = JSON.parse(req.responseText);
          var currentCode = response.current_weather.weathercode;
          var tomorrowCode = response.daily.weathercode[1];

          function mapCodeToEnum(code) {
            if (code === 0) return 0;
            if (code === 1 || code === 2 || code === 3 || code === 45 || code === 48) return 1;
            if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return 2;
            if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) return 3;
            if (code >= 95 && code <= 99) return 4;
            return 0;
          }

          Pebble.sendAppMessage({
            'WeatherStatus': mapCodeToEnum(currentCode),
            'WeatherStatusTomorrow': mapCodeToEnum(tomorrowCode)
          });
        }
      };
      req.send(null);
    },
    function(err) { console.log('Error requesting location!'); },
    { timeout: 15000, maximumAge: 60000 }
  );
}

Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
  // スマホと時計が繋がった瞬間に、「今の設定データを全部教えて！」と合図を送る
  Pebble.sendAppMessage({ 'RequestSettings': 1 });
  // ※ 時計側が画像を持っていない場合は、時計から RequestBgImage が届くので、
  // ここでの無条件 sendSavedBgTarget(0) は不要（二重送信＆真っ白バグ防止）
});

Pebble.addEventListener('appmessage', function(e) {
  var dict = e.payload;

  // 時計から「設定データ（Themeなど）」が送られてきた場合の処理
  if (dict['Theme'] !== undefined) {
    console.log('Settings received from watch!');
    
    var claySettings = {};
    for (var key in dict) {
      if (key !== 'RequestSettings') {
        claySettings[key] = dict[key];
      }
    }
    
    localStorage.setItem('clay-settings', JSON.stringify(claySettings));
  } else if (dict['RequestBgImage'] !== undefined) {
    var target = dict['RequestBgImage'];
    sendSavedBgTarget(target);
  }
  // 設定データではない通信（毎時0分の合図など）が来た場合は天気を更新！
  else {
    fetchWeather();
  }
});
