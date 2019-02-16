var fs = require('fs');
var process = require('process');

var DXC = require('./dxcompiler.js');
var SPIRV = require('./spirv.js');

var content;

var leftToInit = 1;

let dxc = DXC();
let spirv = SPIRV();

spirv.onRuntimeInitialized = () =>
{
  if (--leftToInit == 0) init();
}

dxc.onRuntimeInitialized = () =>
{
	console.log('dxc loaded');
  if (--leftToInit == 0) init();
}

fs.readFile(process.argv[2], (e, data) => 
{
  content = data;
  if (--leftToInit == 0) init();
});

function init()
{
  console.log('init()');
  var options = {
	filename: 'test.hlsl',
	entryPoint: 'main',
	profile: 'ps_6_4',
	args: ['-spirv' ],
	defines: [ ]
  }; 

  var start = new Date().getTime();
  console.log('start:', start);

  const iterations = 5000;
	
  for(var i = 0; i < iterations; ++i)
  {
    var res = compile(content, options);
  }

  var end = new Date().getTime();

  console.log('end:', end);

  console.log('duration:', (end - start) / iterations / 1000.0);
}

function compile(content, options)
{
  var result = { success: false };

  var compileResult = dxc.Compile(content, options);

  if (!compileResult.success)
  {
    console.error('failed to compile HLSL:', compileResult.error);
    result.error = compileResult.error;
    return result;
  }

  var ptr = spirv._malloc(compileResult.data.length);
  
  var array = new Uint8Array(spirv.HEAPU8.buffer, ptr, compileResult.data.length);

  array.set(compileResult.data);

  var data = compileResult.data;

  var res = spirv.SPIRVtoGLSL(array, { reflect: true });

  spirv._free(ptr);

  if (!res.success)
  {
    result.success = false;
    result.error = res.error;
  }
  else
  {
    result.success = true;
    result.glsl = res.glsl;
  }

  return result;
}

