module.exports = function(minified) {
  var clayConfig = this;

  try {
    var platform = 'basalt';
    var model = '';
    if (clayConfig.meta && clayConfig.meta.activeWatchInfo) {
      if (clayConfig.meta.activeWatchInfo.platform) {
        platform = clayConfig.meta.activeWatchInfo.platform.toLowerCase();
      }
      if (clayConfig.meta.activeWatchInfo.model) {
        model = clayConfig.meta.activeWatchInfo.model.toLowerCase();
      }
    }

    var targetW = 144;
    var targetH = 168;
    var isRound = false;

    if (platform === 'chalk' || model.indexOf('round') !== -1) {
      targetW = 180;
      targetH = 180;
      isRound = true;
    } else if (platform === 'emery' || platform === 'gabbro') {
      targetW = 200;
      targetH = 228;
    }

    // 64色Floyd-Steinbergディザリング (ES5準拠)
    function dither(ctx, w, h) {
      var imgData = ctx.getImageData(0, 0, w, h);
      var d = imgData.data;

      for (var y = 0; y < h; y++) {
        for (var x = 0; x < w; x++) {
          var i = (y * w + x) * 4;
          for (var c = 0; c < 3; c++) {
            var oldVal = d[i + c];
            var newVal = Math.round(oldVal / 85) * 85;
            if (newVal > 255) newVal = 255;
            if (newVal < 0) newVal = 0;
            var err = oldVal - newVal;
            d[i + c] = newVal;

            if (x + 1 < w) d[i + 4 + c] += (err * 7) >> 4;
            if (x - 1 >= 0 && y + 1 < h) d[i + (w - 1) * 4 + c] += (err * 3) >> 4;
            if (y + 1 < h) d[i + w * 4 + c] += (err * 5) >> 4;
            if (x + 1 < w && y + 1 < h) d[i + (w + 1) * 4 + c] += (err * 1) >> 4;
          }
        }
      }
      ctx.putImageData(imgData, 0, 0);
    }

    function initCropper(containerId, labelTitle) {
      var container = document.getElementById(containerId);
      if (!container) {
        container = document.querySelector('[id*="' + containerId + '"]');
      }
      if (!container) return;
      container.innerHTML = '';

      var box = document.createElement('div');
      box.style.background = '#1a1a1a';
      box.style.border = '1px solid #444';
      box.style.borderRadius = '6px';
      box.style.padding = '12px';
      box.style.marginTop = '8px';
      box.style.marginBottom = '12px';
      box.style.color = '#fff';

      var titleDiv = document.createElement('div');
      titleDiv.style.fontWeight = 'bold';
      titleDiv.style.marginBottom = '4px';
      titleDiv.style.fontSize = '14px';
      titleDiv.innerText = labelTitle;
      box.appendChild(titleDiv);

      var modelDiv = document.createElement('div');
      modelDiv.style.fontSize = '11px';
      modelDiv.style.color = '#ff4700';
      modelDiv.style.marginBottom = '10px';
      var shapeText = isRound ? 'Round (' + targetW + 'x' + targetH + ')' : 'Rectangular (' + targetW + 'x' + targetH + ')';
      modelDiv.innerText = 'Connected: Pebble ' + platform.toUpperCase() + ' • ' + shapeText;
      box.appendChild(modelDiv);

      // ファイル入力
      var fileInput = document.createElement('input');
      fileInput.type = 'file';
      fileInput.accept = 'image/*';
      fileInput.style.width = '100%';
      fileInput.style.marginBottom = '10px';
      fileInput.style.fontSize = '12px';
      box.appendChild(fileInput);

      // Canvas コンテナ
      var canvasWrap = document.createElement('div');
      canvasWrap.style.textAlign = 'center';
      canvasWrap.style.marginBottom = '8px';

      var canvasBox = document.createElement('div');
      canvasBox.style.display = 'inline-block';
      canvasBox.style.position = 'relative';
      canvasBox.style.border = '2px solid #666';
      canvasBox.style.borderRadius = isRound ? '50%' : '4px';
      canvasBox.style.overflow = 'hidden';
      canvasBox.style.cursor = 'move';
      canvasBox.style.background = '#000';
      canvasBox.style.boxShadow = '0 2px 8px rgba(0,0,0,0.5)';

      var previewCanvas = document.createElement('canvas');
      previewCanvas.width = targetW;
      previewCanvas.height = targetH;
      previewCanvas.style.display = 'block';
      canvasBox.appendChild(previewCanvas);

      // ガイド枠線オーバーレイ用の透明キャンバス
      var overlayCanvas = document.createElement('canvas');
      overlayCanvas.width = targetW;
      overlayCanvas.height = targetH;
      overlayCanvas.style.position = 'absolute';
      overlayCanvas.style.top = '0';
      overlayCanvas.style.left = '0';
      overlayCanvas.style.pointerEvents = 'none';
      canvasBox.appendChild(overlayCanvas);

      canvasWrap.appendChild(canvasBox);
      box.appendChild(canvasWrap);

      var hintDiv = document.createElement('div');
      hintDiv.style.fontSize = '11px';
      hintDiv.style.color = '#888';
      hintDiv.style.textAlign = 'center';
      hintDiv.style.marginBottom = '10px';
      hintDiv.innerText = 'Drag image on screen to adjust position';
      box.appendChild(hintDiv);

      // コントロール行: ズームスライダー
      var zoomRow = document.createElement('div');
      zoomRow.style.marginBottom = '8px';
      var zoomLabel = document.createElement('label');
      zoomLabel.style.display = 'block';
      zoomLabel.style.fontSize = '12px';
      zoomLabel.style.marginBottom = '2px';
      zoomLabel.innerText = 'Zoom Scale:';
      zoomRow.appendChild(zoomLabel);

      var zoomSlider = document.createElement('input');
      zoomSlider.type = 'range';
      zoomSlider.min = '10';
      zoomSlider.max = '300';
      zoomSlider.value = '100';
      zoomSlider.style.width = '100%';
      zoomRow.appendChild(zoomSlider);

      // コントロール行: 明るさスライダー
      var brightRow = document.createElement('div');
      brightRow.style.marginBottom = '8px';
      var brightLabel = document.createElement('label');
      brightLabel.style.display = 'block';
      brightLabel.style.fontSize = '12px';
      brightLabel.style.marginBottom = '2px';
      brightLabel.innerText = 'Brightness Adjust:';
      brightRow.appendChild(brightLabel);

      var brightSlider = document.createElement('input');
      brightSlider.type = 'range';
      brightSlider.min = '-100';
      brightSlider.max = '100';
      brightSlider.value = '0';
      brightSlider.style.width = '100%';
      brightRow.appendChild(brightSlider);

      // ガイド表示チェックボックス
      var checkRow = document.createElement('div');
      checkRow.style.marginBottom = '8px';
      checkRow.style.fontSize = '12px';

      var guideCheck = document.createElement('input');
      guideCheck.type = 'checkbox';
      guideCheck.id = containerId + '_guide';
      guideCheck.checked = true;
      guideCheck.style.marginRight = '6px';

      var guideLabel = document.createElement('label');
      guideLabel.htmlFor = containerId + '_guide';
      guideLabel.innerText = 'Show Watch Face & Aperture Guide Overlay';
      checkRow.appendChild(guideCheck);
      checkRow.appendChild(guideLabel);

      // ディザリングトグル
      var ditherRow = document.createElement('div');
      ditherRow.style.marginBottom = '12px';
      ditherRow.style.fontSize = '12px';

      var ditherCheck = document.createElement('input');
      ditherCheck.type = 'checkbox';
      ditherCheck.id = containerId + '_dither';
      ditherCheck.checked = true;
      ditherCheck.style.marginRight = '6px';

      var ditherLabel = document.createElement('label');
      ditherLabel.htmlFor = containerId + '_dither';
      ditherLabel.innerText = 'Preview 64-Color Floyd-Steinberg Dithering';
      ditherRow.appendChild(ditherCheck);
      ditherRow.appendChild(ditherLabel);

      // スライダー誤タップ防止アコーディオン（折りたたみ）
      var fineTuneDetails = document.createElement('details');
      fineTuneDetails.style.margin = '12px 0';
      fineTuneDetails.style.padding = '10px 14px';
      fineTuneDetails.style.background = 'rgba(128, 128, 128, 0.12)';
      fineTuneDetails.style.borderRadius = '8px';
      fineTuneDetails.style.border = '1px solid rgba(128, 128, 128, 0.25)';
      fineTuneDetails.style.color = 'inherit';

      var fineTuneSummary = document.createElement('summary');
      fineTuneSummary.style.cursor = 'pointer';
      fineTuneSummary.style.fontWeight = 'bold';
      fineTuneSummary.style.fontSize = '13px';
      fineTuneSummary.style.color = 'inherit';
      fineTuneSummary.style.outline = 'none';
      fineTuneSummary.innerText = '⚙️ Fine Tune Controls (Zoom & Brightness) ▼';
      fineTuneDetails.appendChild(fineTuneSummary);

      var fineTuneBody = document.createElement('div');
      fineTuneBody.style.marginTop = '10px';
      fineTuneBody.appendChild(zoomRow);
      fineTuneBody.appendChild(brightRow);
      fineTuneBody.appendChild(checkRow);
      fineTuneBody.appendChild(ditherRow);

      fineTuneDetails.appendChild(fineTuneBody);
      box.appendChild(fineTuneDetails);

      // クリアボタン
      var btnRow = document.createElement('div');
      btnRow.style.textAlign = 'right';
      btnRow.style.marginTop = '10px';

      var clearBtn = document.createElement('button');
      clearBtn.type = 'button';
      clearBtn.style.background = '#c62828';
      clearBtn.style.color = '#fff';
      clearBtn.style.border = 'none';
      clearBtn.style.borderRadius = '4px';
      clearBtn.style.padding = '6px 12px';
      clearBtn.style.fontSize = '12px';
      clearBtn.style.cursor = 'pointer';
      clearBtn.innerText = 'Clear Image';
      btnRow.appendChild(clearBtn);
      box.appendChild(btnRow);

      container.appendChild(box);

      // 内部ステート
      var pCtx = previewCanvas.getContext('2d');
      var oCtx = overlayCanvas.getContext('2d');
      var sourceImg = null;
      var sourceDataUrl = null;
      var posX = 0;
      var posY = 0;
      var scale = 1.0;
      var brightness = 0;
      var isCleared = false;
      var isModified = false;
      var isDragging = false;
      var startX = 0;
      var startY = 0;

      function drawGuideOverlay() {
        oCtx.clearRect(0, 0, targetW, targetH);
        if (!guideCheck.checked) return;

        var cx = targetW / 2;
        var cy = targetH / 2;
        var radius = Math.min(targetW, targetH) * 0.50;

        oCtx.save();
        oCtx.strokeStyle = 'rgba(255, 255, 255, 0.7)';
        oCtx.lineWidth = 1.5;
        if (oCtx.setLineDash) oCtx.setLineDash([4, 4]);
        oCtx.beginPath();
        oCtx.arc(cx, cy, radius, 0, Math.PI * 2);
        oCtx.stroke();

        oCtx.fillStyle = 'rgba(0, 0, 0, 0.35)';
        oCtx.beginPath();
        oCtx.rect(0, 0, targetW, targetH);
        oCtx.arc(cx, cy, radius, 0, Math.PI * 2, true);
        oCtx.fill();

        oCtx.fillStyle = 'rgba(255, 255, 255, 0.9)';
        oCtx.font = 'bold 36px monospace';
        oCtx.textAlign = 'center';
        oCtx.textBaseline = 'middle';
        oCtx.fillText('10:09', cx, cy - 8);

        oCtx.font = '10px sans-serif';
        oCtx.fillText('FRI 30 OCT', cx, cy - radius + 12);
        oCtx.fillText('8.5K / 24°', cx, cy + radius - 12);
        oCtx.restore();
      }

      function render() {
        pCtx.fillStyle = '#000';
        pCtx.fillRect(0, 0, targetW, targetH);

        if (sourceImg && !isCleared) {
          pCtx.save();
          var dw = sourceImg.width * scale;
          var dh = sourceImg.height * scale;
          var dx = (targetW - dw) / 2 + posX;
          var dy = (targetH - dh) / 2 + posY;
          pCtx.drawImage(sourceImg, dx, dy, dw, dh);
          pCtx.restore();

          if (brightness !== 0) {
            var imgData = pCtx.getImageData(0, 0, targetW, targetH);
            var d = imgData.data;
            for (var i = 0; i < d.length; i += 4) {
              d[i] = Math.min(255, Math.max(0, d[i] + brightness));
              d[i+1] = Math.min(255, Math.max(0, d[i+1] + brightness));
              d[i+2] = Math.min(255, Math.max(0, d[i+2] + brightness));
            }
            pCtx.putImageData(imgData, 0, 0);
          }

          if (ditherCheck.checked) {
            dither(pCtx, targetW, targetH);
          }
        } else {
          pCtx.fillStyle = '#666';
          pCtx.font = '12px sans-serif';
          pCtx.textAlign = 'center';
          pCtx.fillText('No Image Selected', targetW / 2, targetH / 2);
        }

        drawGuideOverlay();
      }

      function getImgData() {
        if (!isModified) return null; // 変更がない場合は無駄な画像再送を行わない
        if (isCleared) return { clear: true };
        if (!sourceImg) return null;

        var exportCanvas = document.createElement('canvas');
        exportCanvas.width = targetW;
        exportCanvas.height = targetH;
        var eCtx = exportCanvas.getContext('2d');

        var dw = sourceImg.width * scale;
        var dh = sourceImg.height * scale;
        var dx = (targetW - dw) / 2 + posX;
        var dy = (targetH - dh) / 2 + posY;
        eCtx.drawImage(sourceImg, dx, dy, dw, dh);

        if (brightness !== 0) {
          var imgData = eCtx.getImageData(0, 0, targetW, targetH);
          var d = imgData.data;
          for (var i = 0; i < d.length; i += 4) {
            d[i] = Math.min(255, Math.max(0, d[i] + brightness));
            d[i+1] = Math.min(255, Math.max(0, d[i+1] + brightness));
            d[i+2] = Math.min(255, Math.max(0, d[i+2] + brightness));
          }
          eCtx.putImageData(imgData, 0, 0);
        }

        dither(eCtx, targetW, targetH);

        var finalData = eCtx.getImageData(0, 0, targetW, targetH).data;
        var numPixels = targetW * targetH;
        var binary = '';
        var CHUNK = 4096;
        var chunkArr = [];

        for (var p = 0; p < numPixels; p++) {
          var idx = p * 4;
          var r = finalData[idx];
          var g = finalData[idx + 1];
          var b = finalData[idx + 2];
          var r2 = Math.round(r / 85); if (r2 > 3) r2 = 3; if (r2 < 0) r2 = 0;
          var g2 = Math.round(g / 85); if (g2 > 3) g2 = 3; if (g2 < 0) g2 = 0;
          var b2 = Math.round(b / 85); if (b2 > 3) b2 = 3; if (b2 < 0) b2 = 0;
          var argb = 0xC0 | (r2 << 4) | (g2 << 2) | b2;
          chunkArr.push(argb);
          if (chunkArr.length >= CHUNK) {
            binary += String.fromCharCode.apply(null, chunkArr);
            chunkArr = [];
          }
        }
        if (chunkArr.length > 0) {
          binary += String.fromCharCode.apply(null, chunkArr);
        }

        return {
          b64: btoa(binary),
          w: targetW,
          h: targetH
        };
      }

      function getDraftData() {
        if (isCleared || !sourceImg || !sourceDataUrl) return null;
        return {
          src: sourceDataUrl,
          scale: scale,
          posX: posX,
          posY: posY,
          brightness: brightness
        };
      }

      fileInput.addEventListener('change', function(e) {
        var file = e.target.files[0];
        if (!file) return;

        var reader = new FileReader();
        reader.onload = function(evt) {
          var tempImg = new Image();
          tempImg.onload = function() {
            var maxDim = 400;
            var w = tempImg.width;
            var h = tempImg.height;
            if (w > maxDim || h > maxDim) {
              if (w > h) {
                h = Math.round((h * maxDim) / w);
                w = maxDim;
              } else {
                w = Math.round((w * maxDim) / h);
                h = maxDim;
              }
            }
            var downCanvas = document.createElement('canvas');
            downCanvas.width = w;
            downCanvas.height = h;
            var dCtx = downCanvas.getContext('2d');
            dCtx.drawImage(tempImg, 0, 0, w, h);

            var optimizedDataUrl = downCanvas.toDataURL('image/jpeg', 0.85);
            var img = new Image();
            img.onload = function() {
              sourceImg = img;
              sourceDataUrl = optimizedDataUrl;
              isCleared = false;
              isModified = true;
              var sw = targetW / img.width;
              var sh = targetH / img.height;
              scale = Math.max(sw, sh);
              zoomSlider.value = Math.round(scale * 100);
              brightSlider.value = '0';
              brightness = 0;
              posX = 0;
              posY = 0;
              render();
            };
            img.src = optimizedDataUrl;
          };
          tempImg.src = evt.target.result;
        };
        reader.readAsDataURL(file);
      });

      zoomSlider.addEventListener('input', function() {
        scale = parseInt(this.value, 10) / 100.0;
        isModified = true;
        render();
      });

      brightSlider.addEventListener('input', function() {
        brightness = parseInt(this.value, 10);
        isModified = true;
        render();
      });

      guideCheck.addEventListener('change', drawGuideOverlay);
      ditherCheck.addEventListener('change', render);

      clearBtn.addEventListener('click', function() {
        sourceImg = null;
        sourceDataUrl = null;
        isCleared = true;
        isModified = true;
        fileInput.value = '';
        render();
      });

      // ドラッグ操作
      function onStart(cx, cy) {
        if (!sourceImg || isCleared) return;
        isDragging = true;
        startX = cx - posX;
        startY = cy - posY;
      }
      function onMove(cx, cy) {
        if (!isDragging) return;
        posX = cx - startX;
        posY = cy - startY;
        isModified = true;
        render();
      }
      function onEnd() { 
        isDragging = false;
      }

      canvasBox.addEventListener('mousedown', function(e) { onStart(e.clientX, e.clientY); });
      window.addEventListener('mousemove', function(e) { onMove(e.clientX, e.clientY); });
      window.addEventListener('mouseup', onEnd);

      canvasBox.addEventListener('touchstart', function(e) {
        if (e.touches.length === 1) onStart(e.touches[0].clientX, e.touches[0].clientY);
      }, false);
      window.addEventListener('touchmove', function(e) {
        if (isDragging && e.touches.length === 1) onMove(e.touches[0].clientX, e.touches[0].clientY);
      }, false);
      window.addEventListener('touchend', onEnd);

      // 保存済みドラフト（PKJS meta.userData または WebView localStorage）から復元
      var savedDraft = null;
      if (clayConfig.meta && clayConfig.meta.userData) {
        if (containerId === 'bg_normal_container') savedDraft = clayConfig.meta.userData.normalDraft;
        else if (containerId === 'bg_biz_container') savedDraft = clayConfig.meta.userData.bizDraft;
      }
      if (!savedDraft) {
        try {
          var draftKey = (containerId === 'bg_normal_container') ? 'iris_bg_normal_draft' : 'iris_bg_biz_draft';
          var localStored = localStorage.getItem(draftKey);
          if (localStored) savedDraft = JSON.parse(localStored);
        } catch (e) {}
      }

      if (savedDraft && savedDraft.src) {
        var restoreImg = new Image();
        restoreImg.onload = function() {
          sourceImg = restoreImg;
          sourceDataUrl = savedDraft.src;
          scale = typeof savedDraft.scale !== 'undefined' ? savedDraft.scale : 1.0;
          posX = savedDraft.posX || 0;
          posY = savedDraft.posY || 0;
          brightness = savedDraft.brightness || 0;
          zoomSlider.value = Math.round(scale * 100);
          brightSlider.value = brightness.toString();
          isCleared = false;
          render();
        };
        restoreImg.src = savedDraft.src;
      } else {
        render();
      }

      container.getImgData = getImgData;
      container.getDraftData = getDraftData;
      container.isCleared = function() { return isCleared; };
    }

    // ページの構築完了（AFTER_BUILD）を待ってからDOMを生成・注入！
    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
      initCropper('bg_normal_container', 'Normal Background Image');

      // 通常背景スタイルの連動（Custom Photo 選択時のみ画像アップローダーを表示）
      var themeSelect = clayConfig.getItemByMessageKey && clayConfig.getItemByMessageKey('Theme');
      if (themeSelect) {
        var normBox = document.getElementById('bg_normal_container');
        var normSection = normBox ? normBox.closest('.section') : null;
        function updateNormalVisibility() {
          var val = themeSelect.get();
          var isPhoto = (val === '2' || val === 2);
          if (normBox) normBox.style.display = isPhoto ? 'block' : 'none';
          if (normSection) normSection.style.display = isPhoto ? 'block' : 'none';
        }
        themeSelect.on('change', updateNormalVisibility);
        updateNormalVisibility();
      }

      // フォーム送信（submit）をキャプチャして画像とドラフトデータを確実に含めて送信！
      var mainForm = document.getElementById('main-form');
      if (mainForm) {
        mainForm.addEventListener('submit', function(e) {
          e.stopImmediatePropagation();
          e.preventDefault();

          var res = clayConfig.serialize();
          var norm = document.getElementById('bg_normal_container');

          if (norm && norm.getImgData) {
            var d = norm.getImgData();
            if (d) res.iris_bg_normal = d;
          }
          if (norm && norm.getDraftData) {
            res.iris_bg_normal_draft = norm.getDraftData();
          }

          try {
            if (res.iris_bg_normal_draft) {
              localStorage.setItem('iris_bg_normal_draft', JSON.stringify(res.iris_bg_normal_draft));
            } else if (norm && norm.isCleared && norm.isCleared()) {
              localStorage.removeItem('iris_bg_normal_draft');
            }
          } catch (storageErr) {}

          var returnTo = 'pebblejs://close#';
          location.href = returnTo + encodeURIComponent(JSON.stringify(res));
        }, true);
      }
    });
  } catch (err) {
    console.error('Error in custom-clay:', err);
  }
};
