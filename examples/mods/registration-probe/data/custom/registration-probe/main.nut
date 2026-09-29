// API self-check only. No actors spawned and no level/menu replaced.
mod.Register("enemy", "probe", {
    name = "Registration probe",
    create = function(value) { return value + 1; }
});
if (KinokoMods.Create("enemy", "registration-probe:probe", [41]) != 42)
    throw "Mod content factory self-check failed";

// A visible launcher/API demonstration, not a playable level.
mod.Register("stage", "launcher-demo", {
    name = "Launcher demo (returns to title)",
    create = function() {
        MessageBox("The selected Mod stage callback ran successfully. This demo returns to the original title screen.");
    }
});
