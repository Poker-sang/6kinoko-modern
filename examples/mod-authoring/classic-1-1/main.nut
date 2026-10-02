// Keep original world/title/player flow. Only slot 16's label is extended.
local originalSetWorld = PlayerStatus.SetWorld;
PlayerStatus.SetWorld = function(worldNo, subNo, stageId) : (originalSetWorld) {
    originalSetWorld.call(this, worldNo, subNo, stageId == "c16a" ? "c01a" : stageId);
    if (stageId == "c16a") {
        this.stage = stageId;
        this.wmap_stage.AssociateResource(this.resource.mod_stage16);
    }
}.bindenv(PlayerStatus.global);

// Original symbol remains as the clear/unlock state carrier. Only its alpha
// is suppressed; a separate resource-backed layer renders the green balloon.
local world = WorldMap.global;
local originalInitSymbol = world.InitSymbol;
world.InitSymbol = function() : (originalInitSymbol) {
    originalInitSymbol.call(this);
    local index = this.symbol.layout.GetChipByPosition(80, 816);
    this.symbol.layout.GetChipLayout(index).alpha = 0.0;
};
local originalAnimation = world.UpdateChipAnimation;
world.UpdateChipAnimation = function() : (originalAnimation) {
    originalAnimation.call(this);
    local index = this.symbol.layout.GetChipByPosition(80, 816);
    local state = this.symbol.layout.GetChipLayout(index);
    this.symbol_mod.layout.GetChipLayout(0).visible = state.visible;
    state.alpha = 0.0;
    local symbols = this.symbol;
    this.symbol = this.symbol_mod;
    try { originalAnimation.call(this); }
    catch (error) { this.symbol = symbols; throw error; }
    this.symbol = symbols;
};
