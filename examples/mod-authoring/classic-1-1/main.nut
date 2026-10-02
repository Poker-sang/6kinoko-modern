// Keep original world/title/player flow. Only slot 16's label is extended.
local originalSetWorld = PlayerStatus.SetWorld;
PlayerStatus.SetWorld = function(worldNo, subNo, stageId) : (originalSetWorld) {
    originalSetWorld.call(this, worldNo, subNo, stageId == "c16a" ? "c01a" : stageId);
    if (stageId == "c16a") {
        this.stage = stageId;
        this.wmap_stage.AssociateResource(this.resource.mod_stage16);
    }
};
