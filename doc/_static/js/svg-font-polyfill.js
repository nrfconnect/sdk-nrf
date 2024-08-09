
/* List of fonts that will be loaded from an external file. */
let svgExternalFontList = [
	{
	  name: 'Calibri',
	  //url: 'https://fonts.googleapis.com/css?family=Calibri:400,700,400italic,700italic',
    url: 'https://fonts.googleapis.com/css2?family=Carlito:ital,wght@0,400;0,700;1,400;1,700&display=swap',
	  checkSystem: true,
    replaceFrom: 'Carlito',
    replaceTo: 'Calibri',
	}
      ];
      
      addEventListener("DOMContentLoaded", () => {
      
	const isFontAvailable = (function () {
	  var width;
	  var body = document.body;
      
	  var container = document.createElement('span');
	  container.innerHTML = Array(10).join('Wi_$.');
	  container.style.cssText = [
	    'position:absolute',
	    'width:auto',
	    'font-size:40px',
	    'left:-99999px',
	    'top:-99999px'
	  ].join(' !important;');
      
	  var getWidth = function (fontFamily) {
	    container.style.fontFamily = fontFamily;
      
	    body.appendChild(container);
	    width = container.clientWidth;
	    body.removeChild(container);
      
	    return width;
	  };
      
	  // Pre compute the widths of monospace, serif & sans-serif
	  // to improve performance.
	  var monoWidth = getWidth('monospace');
	  var serifWidth = getWidth('serif');
	  var sansWidth = getWidth('sans-serif');
	  console.log(monoWidth, serifWidth, sansWidth);
      
	  return function (font) {
	    return monoWidth !== getWidth(font + ',monospace') ||
	      sansWidth !== getWidth(font + ',sans-serif') ||
	      serifWidth !== getWidth(font + ',serif');
	  };
	})();
      
  function escapeRegExp(text) {
      return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  }

	setTimeout(() => {
	  console.log('Times New Roman', isFontAvailable('Times New Roman'));
	  console.log('Arial', isFontAvailable('Arial'));
	  console.log('Calibri', isFontAvailable('Calibri'));
	  console.log('Cambria', isFontAvailable('Cambria'));
	  console.log('Aptos', isFontAvailable('Aptos'));
	  console.log('Liberation Serif', isFontAvailable('Liberation Serif'));
	  console.log('Ubuntu', isFontAvailable('Ubuntu'));
	  console.log('DejaVu Sans', isFontAvailable('DejaVu Sans'));
	}, 2000);
      
	svgExternalFontList = svgExternalFontList.filter(info => {
	  return !info.checkSystem || !isFontAvailable(info.name);
	});
      
	if (svgExternalFontList.length === 0) {
	  return;
	}
      
	const fontRexExp = new RegExp(`\\b(${svgExternalFontList.map(x => escapeRegExp(x.name)).join('|')})\\b`, 'gi');
      
	let fontByName = Object.create(null);
	for (let f of svgExternalFontList) {
	  fontByName[f.name.toLowerCase()] = f;
	}

  async function toDataURL(data, type) {
    return await new Promise(r => {
	      const reader = new FileReader();
	      reader.onload = () => r(reader.result);
	      reader.readAsDataURL(new File([data], 'file', { type }));
	    });
  }

  async function urlFromEntry(entry) {
    if (entry.replaceFrom && entry.replaceTo) {
      let res = await fetch(entry.url);
      type = res.headers.get('Content-Type') || 'text/css';
      let data = await res.text();
      let regexp = new RegExp(`\\b${escapeRegExp(entry.replaceFrom)}\\b`, 'g');
      data = data.replace(regexp, entry.replaceTo);
      //console.log('--------------------------------', data);
      return toDataURL(data, type);
    } else {
      return entry.url;
    }
  }

	async function processImage(img) {
	  try {
	    //img.parentElement.style.backgroundColor = 'red';
	    let data = await (await fetch(img.src)).text();
	    let fontUsed = new Set([...data.matchAll(fontRexExp)].map(m => m[1].toLowerCase()));
	    //img.style.border = '1px solid black';
	    //await new Promise((resolve, reject) => setTimeout(resolve, 1000));
	    //console.log(fontUsed);
	    //console.log(fontByName);
      let importLines = [];
      for (let fontName of fontUsed) {
        let font = fontByName[fontName];
        //console.log('-----------------', fontName, font);
        let url = await urlFromEntry(font);
        importLines.push(`@import url('${url}');`);
      }
	    let cssImports = importLines.join('\n');
	    if (!cssImports) return;
	    let div = document.createElement('iframe');
	    //div.style.display = 'block';
	    div.style.width = img.offsetWidth + 'px';
	    div.style.height = img.offsetHeight + 'px';
	    div.style.margin = '0px';
	    div.style.padding = '0px';
	    //div.style.border = '1px solid black';
	    div.style.border = 'none';
	    div.style.isolation = 'isolate';
	    div.style.overflow = 'hidden';
	    div.style.verticalAlign = 'middle';
	    //div.style.position = 'absolute';
	    //div.style.top = '-9999px';
	    //div.style.left = '-9999px';
	    const svgDoc = new DOMParser().parseFromString(data, 'image/svg+xml');
	    let svg = svgDoc.getElementsByTagName('svg')[0];
	    svg.width = img.offsetWidth + 'px';
	    svg.height = img.offsetHeight + 'px';
	    svg.style.width = img.offsetWidth + 'px';
	    svg.style.height = img.offsetHeight + 'px';
	    let style = svgDoc.getElementsByTagName('style')[0];
	    style.textContent = cssImports + '\n' + style.textContent;
	    data = new XMLSerializer().serializeToString(svgDoc);
	    //console.log(dataURL);
	    //div.style.backgroundImage = `url('${img.src}')`;
      //console.log(data);
	    div.srcdoc = `<html><body style="overflow: hidden; padding: 0px; margin: 0px;">${data}</body></html>`;
	    //div.src = dataURL;
	    /*div.onload = () => {
	      setTimeout(() => {
	      div.style.position = '';
	      div.style.top = '';
	      div.style.left = '';
	      img.parentNode.replaceChild(div, img);
	      }, 100);
	    };*/
	    img.style.position = 'absolute';
	    img.parentNode.insertBefore(div, img.nextSibling);
	    setTimeout(() => {
	      img.parentNode.removeChild(img);
	    }, 50);
	    //img.parentNode.replaceChild(div, img);
	    //document.body.appendChild(div);
	  } catch (e) {
	    console.error('Error processing image: ', img.src, e);
	  }
	}
      
	for (let img of document.querySelectorAll("img")) {
	  if (img.src && img.src.toLowerCase().endsWith('.svg')) {
	    if (img.complete) {
	      processImage(img);
	    } else {
	      const callback = () => {
		img.removeEventListener("load", callback);
		processImage(img);
	      };
	      img.addEventListener("load", callback);
	    }
	  }
	}
      });