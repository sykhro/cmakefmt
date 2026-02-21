let formatTimer;

let cmConfig, cmSource, cmOutput;

// The global Module object configured by Emscripten
var Module = {
    onRuntimeInitialized: function() {
        document.getElementById('loading').classList.add('hidden');
        
        cmConfig = CodeMirror.fromTextArea(document.getElementById('configInput'), {
            mode: "yaml",
            lineNumbers: true
        });

        cmSource = CodeMirror.fromTextArea(document.getElementById('sourceInput'), {
            mode: "cmake",
            lineNumbers: true
        });

        cmOutput = CodeMirror.fromTextArea(document.getElementById('sourceOutput'), {
            mode: "cmake",
            lineNumbers: true,
            readOnly: true
        });

        try {
            const configPtr = Module.ccall('get_default_config', 'number', [], []);
            if (configPtr) {
                cmConfig.setValue(Module.UTF8ToString(configPtr));
                Module.ccall('free_string', null, ['number'], [configPtr]);
            }
        } catch (e) {
            console.error("Failed to load default config from WASM:", e);
            cmConfig.setValue("IndentWidth: 4\nColumnLimit: 80");
        }

        triggerFormat(); // Initial format
        
        cmSource.on('change', debounceFormat);
        cmConfig.on('change', debounceFormat);

        setupButtons();
    }
};

function setupButtons() {
    document.getElementById('btnCopyOutput').addEventListener('click', () => {
        navigator.clipboard.writeText(cmOutput.getValue()).then(() => {
            const btn = document.getElementById('btnCopyOutput');
            const old = btn.textContent;
            btn.textContent = "Copied!";
            setTimeout(() => btn.textContent = old, 1500);
        });
    });

    const fileConfig = document.getElementById('fileConfig');
    document.getElementById('btnImportConfig').addEventListener('click', () => fileConfig.click());
    fileConfig.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = (e) => cmConfig.setValue(e.target.result);
        reader.readAsText(file);
        e.target.value = ''; // reset
    });

    const fileSource = document.getElementById('fileSource');
    document.getElementById('btnImportSource').addEventListener('click', () => fileSource.click());
    fileSource.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = (e) => cmSource.setValue(e.target.result);
        reader.readAsText(file);
        e.target.value = ''; // reset
    });
}

function debounceFormat() {
    clearTimeout(formatTimer);
    formatTimer = setTimeout(triggerFormat, 200); // 200ms debounce
}

function triggerFormat() {
    const source = cmSource.getValue();
    const config = cmConfig.getValue();

    try {
        const resultPtr = Module.ccall(
            'format_cmake_code',
            'number',
            ['string', 'string'],
            [source, config]
        );

        if (resultPtr) {
            const formattedString = Module.UTF8ToString(resultPtr);
            cmOutput.setValue(formattedString);
            Module.ccall('free_string', null, ['number'], [resultPtr]);
        } else {
            cmOutput.setValue("Error: Formatter returned NULL pointer.");
        }
    } catch (e) {
        console.error(e);
        cmOutput.setValue("Fatal error calling WebAssembly module:\n" + e);
    }
}
