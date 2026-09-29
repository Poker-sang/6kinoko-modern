// API self-check only. No actors spawned and no level/menu replaced.
mod.Register("enemy", "probe", {
    name = "Registration probe",
    create = function(value) { return value + 1; }
});
if (KinokoMods.Create("enemy", "registration-probe:probe", [41]) != 42)
    throw "Mod content factory self-check failed";
