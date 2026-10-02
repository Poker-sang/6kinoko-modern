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
// is suppressed; an extra chip in the same original layer renders green.
local world = WorldMap.global;
local originalInitSymbol = world.InitSymbol;
world.InitSymbol = function() : (originalInitSymbol) {
    originalInitSymbol.call(this);
    local index = this.symbol.layout.GetChipByPosition(80, 848);
    this.symbol.layout.GetChipLayout(index).alpha = 0.0;
};
local originalAnimation = world.UpdateChipAnimation;
world.UpdateChipAnimation = function() : (originalAnimation) {
    originalAnimation.call(this);
    local index = this.symbol.layout.GetChipByPosition(80, 848);
    local state = this.symbol.layout.GetChipLayout(index);
    local layout = this.symbol.layout;
    for (local i = 0; i < layout.chipCount; ++i) {
        local chip = layout.GetChipLayout(i);
        if (chip.chipID == 1148) chip.visible = state.visible;
    }
    state.alpha = 0.0;
    local symbols = this.symbol;
    // Reuse the original frame selection, redirecting only balloon rectangles.
    this.symbol = {layout = {SetChipRect = function(id,x,y,w,h) : (layout) {
        if (id == 1023) layout.SetChipRect(1148,x,y,w,h);
    }}};
    try { originalAnimation.call(this); }
    catch (error) { this.symbol = symbols; throw error; }
    this.symbol = symbols;
};
