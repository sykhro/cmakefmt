let formatTimer;

const sourceInput = document.getElementById('sourceInput');
const configInput = document.getElementById('configInput');
const sourceOutput = document.getElementById('sourceOutput');
const loadingScreen = document.getElementById('loading');

function setupTextEditor(textareaId, linesId) {
    const textarea = document.getElementById(textareaId);
    const lines = document.getElementById(linesId);
    
    function updateLines() {
        const lineCount = textarea.value.split('\n').length;
        let numbers = '';
        for (let i = 1; i <= lineCount; i++) {
            numbers += i + '<br>';
        }
        lines.innerHTML = numbers;

        // update syntax highlighting display underneath
        const displayCode = textarea.parentElement.querySelector('code');
        if (displayCode) {
            // escape html
            displayCode.textContent = textarea.value;
            displayCode.removeAttribute('data-highlighted');
            hljs.highlightElement(displayCode);
        }
    }
    
    textarea.addEventListener('input', updateLines);
    textarea.addEventListener('scroll', () => {
        lines.scrollTop = textarea.scrollTop;
        const displayPre = textarea.parentElement.querySelector('pre');
        if (displayPre) displayPre.scrollTop = textarea.scrollTop;
        if (displayPre) displayPre.scrollLeft = textarea.scrollLeft;
    });
    
    // Tab support
    textarea.addEventListener('keydown', function(e) {
        if (e.key === 'Tab') {
            e.preventDefault();
            const start = this.selectionStart;
            const end = this.selectionEnd;
            this.value = this.value.substring(0, start) + "    " + this.value.substring(end);
            this.selectionStart = this.selectionEnd = start + 4;
            updateLines();
            debounceFormat();
        }
    });
    
    updateLines();
}

// The global Module object configured by Emscripten
var Module = {
    onRuntimeInitialized: function() {
        loadingScreen.classList.add('hidden');
        
        try {
            const configPtr = Module.ccall('get_default_config', 'number', [], []);
            if (configPtr) {
                configInput.value = Module.UTF8ToString(configPtr);
                Module.ccall('free_string', null, ['number'], [configPtr]);
            }
        } catch (e) {
            console.error("Failed to load default config from WASM:", e);
            configInput.value = "IndentWidth: 4\nColumnLimit: 80";
        }

        setupTextEditor('configInput', 'configLineNumbers');
        setupTextEditor('sourceInput', 'inputLineNumbers');
        
        triggerFormat(); // Initial format
        
        sourceInput.addEventListener('input', debounceFormat);
        configInput.addEventListener('input', debounceFormat);
    }
};

function debounceFormat() {
    clearTimeout(formatTimer);
    formatTimer = setTimeout(triggerFormat, 200); // 200ms debounce
}

function triggerFormat() {
    const source = sourceInput.value;
    const config = configInput.value;

    try {
        const resultPtr = Module.ccall(
            'format_cmake_code',
            'number',
            ['string', 'string'],
            [source, config]
        );

        if (resultPtr) {
            const formattedString = Module.UTF8ToString(resultPtr);
            sourceOutput.textContent = formattedString;
            
            sourceOutput.removeAttribute('data-highlighted');
            sourceOutput.className = 'language-cmake';
            
            hljs.highlightElement(sourceOutput);
            hljs.lineNumbersBlock(sourceOutput);
            
            Module.ccall('free_string', null, ['number'], [resultPtr]);
        } else {
            sourceOutput.textContent = "Error: Formatter returned NULL pointer.";
        }
    } catch (e) {
        console.error(e);
        sourceOutput.textContent = "Fatal error calling WebAssembly module:\n" + e;
    }
}
