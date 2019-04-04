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
      const input_ptr = Module._malloc(input.length * input.BYTES_PER_ELEMENT);
      Module.HEAPU8.set(input, input_ptr);
      console.log("input", input, input_ptr);

      const output = new Uint8Array(input.length);
      const output_ptr = Module._malloc(output.length * output.BYTES_PER_ELEMENT);
      Module.HEAPU8.set(output, output_ptr);

      const output_size_list = new Uint32Array(Math.round(input.length / 1024));
      const output_size_list_ptr = Module._malloc(output_size_list.length * output_size_list.BYTES_PER_ELEMENT);
      Module.HEAPU32.set(output_size_list, output_size_list_ptr);

      const sampleRate = new Uint32Array(1);
      sampleRate[0] = 16000;
      const pkgCount = Module.ccall("pcm2aac", "number", ["number", "number", "number", "number", "number"], [sampleRate[0], input_ptr, input.length, output_ptr, output_size_list_ptr]);
      console.log("pkgCount:", pkgCount);

      for (let i = 0; i < pkgCount; i += 1) {
        const frameSize = Module.HEAPU32[output_size_list_ptr / Uint32Array.BYTES_PER_ELEMENT + i];
        output_size_list[i] = frameSize;
      }

      const aacSamples = [];
      let i = 0, j = 0;
      console.log(pkgCount, output_size_list);
      while (j < pkgCount) {
        const frameSize = output_size_list[j];
        const aacSample = Module.HEAPU8.subarray(i + output_ptr / Uint8Array.BYTES_PER_ELEMENT, i + frameSize + output_ptr / Uint8Array.BYTES_PER_ELEMENT);
        aacSamples.push(aacSample);
        i += frameSize;
        j += 1;
      }
      Module._free(input_ptr);
      Module._free(output_ptr);
      Module._free(output_size_list_ptr);

      console.log("aacSamples", aacSamples);
    }

    reader.readAsArrayBuffer(file);
  }
};
