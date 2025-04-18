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
