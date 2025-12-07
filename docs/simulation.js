const simulationState = {
    step: 0,
    tasks: {
        client: { name: 'Client Task', color: 'var(--secondary-color)', type: 'user' },
        server: { name: 'IPC Server', color: 'var(--primary-color)', type: 'user' },
        kernel: { name: 'Kernel (Ring 0)', color: '#ff5f56', type: 'kernel' }
    }
};

function initSimulation() {
    const cpu = document.getElementById('cpu-task');
    const readyQ = document.getElementById('ready-queue');
    const blockedQ = document.getElementById('blocked-queue');
    const log = document.getElementById('sim-log');
    
    if (!cpu || !readyQ || !blockedQ) return;

    // Initial State
    updateVisuals('client', ['server'], []);
    addLog("System started. Scheduler active.");

    let step = 0;
    
    setInterval(() => {
        step = (step + 1) % 6;
        
        switch(step) {
            case 0: // Client Running
                updateVisuals('client', ['server'], []);
                addLog("Client: Generating 'PING' message...");
                break;
            case 1: // Syscall Send
                updateVisuals('kernel', ['server'], ['client']);
                addLog("SYSCALL: sys_send(target=Server, msg='PING')");
                animatePacket('cpu-container', 'blocked-queue'); // Visual flair
                break;
            case 2: // Context Switch
                updateVisuals('kernel', ['server'], ['client']);
                addLog("Scheduler: Client BLOCKED. Switching to Server...");
                break;
            case 3: // Server Running
                updateVisuals('server', [], ['client']);
                addLog("Server: Woke up. Received 'PING'. Processing...");
                break;
            case 4: // Server Reply
                updateVisuals('kernel', ['client'], ['server']);
                addLog("SYSCALL: sys_send(target=Client, msg='PONG')");
                break;
            case 5: // Client Wakes
                updateVisuals('client', ['server'], []);
                addLog("Client: Received 'PONG'. Cycle complete.");
                break;
        }
    }, 2000);
}

function updateVisuals(runningId, readyIds, blockedIds) {
    const cpu = document.getElementById('cpu-task');
    const readyQ = document.getElementById('ready-queue');
    const blockedQ = document.getElementById('blocked-queue');
    
    // Update CPU
    const runningTask = simulationState.tasks[runningId];
    cpu.innerHTML = `<div class="task-card" style="border-color: ${runningTask.color}">
        <span class="task-icon">${runningTask.type === 'kernel' ? '⚙️' : '👤'}</span>
        ${runningTask.name}
    </div>`;
    cpu.style.boxShadow = `0 0 20px ${runningTask.color}40`;

    // Update Ready Queue
    readyQ.innerHTML = readyIds.map(id => {
        const t = simulationState.tasks[id];
        return `<div class="task-mini" style="background: ${t.color}20; color: ${t.color}; border: 1px solid ${t.color}">
            ${t.name}
        </div>`;
    }).join('');

    // Update Blocked Queue
    blockedQ.innerHTML = blockedIds.map(id => {
        const t = simulationState.tasks[id];
        return `<div class="task-mini" style="background: ${t.color}10; color: ${t.color}; border: 1px dashed ${t.color}; opacity: 0.7">
            🔒 ${t.name}
        </div>`;
    }).join('');
}

function addLog(msg) {
    const log = document.getElementById('sim-log');
    const entry = document.createElement('div');
    entry.className = 'log-entry';
    entry.innerHTML = `<span class="log-time">[${new Date().toLocaleTimeString().split(' ')[0]}]</span> ${msg}`;
    log.prepend(entry);
    if (log.children.length > 6) log.lastElementChild.remove();
}

function animatePacket(fromId, toId) {
    // Optional: Add CSS animation for a flying packet
}

function copyCode() {
    const activeTab = document.querySelector('.tab.active');
    let codeId = 'code-ipc'; // default
    
    if (activeTab) {
        const tabName = activeTab.innerText.toLowerCase().split('.')[0]; // ipc, scheduler, syscall
        if (tabName.includes('ipc')) codeId = 'code-ipc';
        else if (tabName.includes('sched')) codeId = 'code-sched';
        else if (tabName.includes('sys')) codeId = 'code-sys';
    }

    const codeBlock = document.getElementById(codeId);
    if (!codeBlock) return;

    const text = codeBlock.innerText;
    navigator.clipboard.writeText(text).then(() => {
        const btn = document.querySelector('.copy-btn');
        const originalHTML = btn.innerHTML;
        
        btn.classList.add('copied');
        btn.innerHTML = `<span>✓</span> Copied!`;
        
        setTimeout(() => {
            btn.classList.remove('copied');
            btn.innerHTML = originalHTML;
        }, 2000);
    });
}

document.addEventListener('DOMContentLoaded', initSimulation);
