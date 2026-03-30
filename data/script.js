// Upload firmware
document.getElementById('uploadForm').onsubmit = e => {
  e.preventDefault();
  const f = document.getElementById('file').files[0];
  if (!f) return;
  const xhr = new XMLHttpRequest();
  xhr.open("POST","/fw",true);
  xhr.upload.onprogress = ev => {
    document.getElementById('progressBar').style.width =
      Math.round(ev.loaded/ev.total*100) + '%';
  };
  xhr.onload = () => {
    document.getElementById('status').innerText = xhr.responseText;
    setTimeout(()=>location.reload(),5000);
  };
  const fd = new FormData();
  fd.append("update", f);
  xhr.send(fd);
  document.getElementById('status').innerText = "Uploading…";
};

// Upload filesystem image
document.getElementById('fsUploadForm').onsubmit = e => {
  e.preventDefault();
  const f = document.getElementById('fsFile').files[0];
  if (!f) return;
  const xhr = new XMLHttpRequest();
  xhr.open("POST","/fs",true);
  xhr.upload.onprogress = ev => {
    document.getElementById('fsProgressBar').style.width =
      Math.round(ev.loaded/ev.total*100) + '%';
  };
  xhr.onload = () => {
    document.getElementById('fsStatus').innerText = xhr.responseText;
    setTimeout(()=>location.reload(),5000);
  };
  const fd = new FormData();
  fd.append("update", f);
  xhr.send(fd);
  document.getElementById('fsStatus').innerText = "Uploading filesystem…";
};

// Load filesystem info
fetch('/status')
  .then(r => r.json())
  .then(data => {
    const used = data.totalSpace - data.freeSpace;
    const total = data.totalSpace;
    const pct = total > 0 ? Math.round(used/total*100) : 0;
    document.getElementById('fsInfo').innerText =
      `Used: ${(used/1024).toFixed(1)} KB / ${(total/1024).toFixed(1)} KB (${pct}%)`;
  })
  .catch(() => {
    document.getElementById('fsInfo').innerText = 'Could not load filesystem info';
  });

// Scan and populate SSID list
document.getElementById('scanBtn').onclick = () => {
  const st = document.getElementById('wifiStatus');
  st.innerText = "Scanning…";
  fetch('/scan')
    .then(r => r.json())
    .then(data => {
      const sel = document.getElementById('ssidList');
      sel.innerHTML = '';
      data.networks.forEach(net => {
        const name = net.split(' (')[0];
        const opt  = document.createElement('option');
        opt.value = name;
        opt.text  = net;
        sel.add(opt);
      });
      st.innerText = "Scan complete!";
    });
};
