Module.print = console.log;
const wasmReady = new Promise(resolve => {
  Module['onRuntimeInitialized'] = () => {
    console.log("wasm ready!");
    resolve();
  };
});
window.onload = () => {
  const input = document.getElementById("file");
  input.onchange = (e) => {
    const file = input.files[0];
    const reader = new FileReader();
    reader.onload = async (e) => {
      const result = e.target.result;
      console.log("result", result);
      await wasmReady;
      const input = new Uint8Array(result);
      const input_buffer = Module._malloc(input.length * input.BYTES_PER_ELEMENT);
      Module.HEAPU8.set(input, input_buffer);
      console.log("input", input, input_buffer);

      const output = new Uint8Array(input.length);
      const output_buffer = Module._malloc(output.length * output.BYTES_PER_ELEMENT);
      Module.HEAPU8.set(output, output_buffer);

      const output_szie_list = new Uint32Array(Math.round(input.length / 1024));
      console.log("output_size_list", output_szie_list);
      const output_szie_list_buffer = Module._malloc(output_szie_list.length * output_szie_list.BYTES_PER_ELEMENT);
      Module.HEAPU32.set(output_szie_list, output_szie_list_buffer);

      const sampleRate = new Uint32Array(1);
      sampleRate[0] = 16000;
      const pkgCount = Module.ccall("pcm2aac", "number", ["number", "number", "number", "number", "number"], [sampleRate[0], input_buffer, input.length, output_buffer, output_szie_list_buffer]);
      let totalSize = 0;
      console.log("pkgCount:", pkgCount);

      for (let i = 0; i < pkgCount; i += 1) {
        const frameSize = Module.HEAPU32[output_szie_list_buffer/Uint32Array.BYTES_PER_ELEMENT + i];
        totalSize += frameSize;
      }

      console.log("total size:", totalSize);
    }

    reader.readAsArrayBuffer(file);
  }
};
