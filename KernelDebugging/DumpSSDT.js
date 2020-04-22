// DumpSSDT.js
//
// Parse the SSDT and expose as array within WinDbg.
//
// Usage:
// kd> .scriptload \path\to\DumpSSDT.js
// kd> dx @$ssdt()
//
// Example:
 // kd> dx @$ssdt().Where(s => s.Name.Contains("nt")).Count()
 
"use strict";

const log = msg => host.diagnostics.debugLog(msg + "\n");
const system = cmd => host.namespace.Debugger.Utility.Control.ExecuteCommand(cmd);
const u32 = x => host.memory.readMemoryValues(x, 1, 4)[0];

function IsX64() {
    return host.namespace.Debugger.State.PseudoRegisters.General.ptrsize == 8;
}

function GetSymbolFromAddress(x) { 
    return system('.printf "%y", ' + x.toString(16)).First(); 
}

class SsdtEntry
{
    constructor(addr, name, argnum)
    {
        this.Address = addr
        this.Name = name;
        this.NumberOfArgumentsOnStack = argnum;
    }

    toString()
    {
        let str = `(${this.Address.toString(16)}) ${this.Name}`;
        if (IsX64() && this.NumberOfArgumentsOnStack)
            str += ` StackArgNum=${this.NumberOfArgumentsOnStack}`;
        return str;
    }
}

function GetSsdtData()
{
    let SsdtData = [];
    if(IsX64())
    {
        let ServiceTable = host.getModuleSymbolAddress("nt", "KeServiceDescriptorTable");
        let NumberOfSyscalls = u32(host.getModuleSymbolAddress("nt", "KiServiceLimit"));
        let expr = "**(unsigned int(**)[" + NumberOfSyscalls.toString() + "])0x" + ServiceTable.toString(16);
        SsdtData["Offsets"] = host.evaluateExpression(expr);
        SsdtData["Base"] = host.getModuleSymbolAddress("nt", "KiServiceTable");
    }
    else
    {
        let ServiceTable = host.getModuleSymbolAddress("nt", "_KeServiceDescriptorTable");
        let NumberOfSyscalls = u32(host.getModuleSymbolAddress("nt", "_KiServiceLimit"));
        let expr = "**(unsigned int(**)[" + NumberOfSyscalls.toString() + "])0x" + ServiceTable.toString(16);
        SsdtData["Offsets"] = host.evaluateExpression(expr);
        SsdtData["Base"] = host.getModuleSymbolAddress("nt", "_KiServiceTable");
    }

    return SsdtData;
}

function *DumpSSDT()
{
    let OffsetTable = GetSsdtData();
    for (var i = 0 ; i < OffsetTable.Offsets.Count(); ++i)
    {
        let Address = 0;
        let ArgNum  = 0;

        if (IsX64())
        {
            Address = OffsetTable.Base.add(OffsetTable.Offsets[i] >> 4);
            ArgNum = OffsetTable.Offsets[i] & 3;
        }
        else
        {
            Address = OffsetTable.Offsets[i];
        }

        var Symbol = GetSymbolFromAddress(Address);
        yield new SsdtEntry(Address, Symbol, ArgNum);
    }
}

function initializeScript()
{
    log("[+] Creating variable `ssdt` for the SSDT...");
    return [
        new host.apiVersionSupport(1, 3),
        new host.functionAlias(
            DumpSSDT,
            "ssdt"
        ),
    ];
}