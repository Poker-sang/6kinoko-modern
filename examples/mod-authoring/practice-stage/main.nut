// Locally generated map copy; original terrain/player/enemy logic is reused.
local practiceMap = "mods/practice-stage.act";
mod.Register("stage", "first-stage", {
    name = "First stage practice (original terrain, 99 lives)",
    create = function() : (practiceMap) {
        local root = getroottable();
        foreach (name in ["InitStage", "InitGlobal", "ChangeStageToWorld", "ChangeStageToTitle",
                          "savedata", "TitleMenu", "WorldMap", "Logo", "PlayerStatus", "Fader2"])
            if (!(name in root)) throw "Practice stage needs original boot: " + name;
        // All saves are already redirected by the Mod launcher. Slot A is kept
        // in the Mod session; ordinary progress is never imported or overwritten.
        ::currentSavedata <- savedata[0].weakref();
        InitGlobal();
        TitleMenu.pl.EndStage();
        WorldMap.pl.EndStage();
        Logo.pl.EndStage();
        local originalWorld = ChangeStageToWorld;
        local originalTitle = ChangeStageToTitle;
        local restore = function() : (originalWorld, originalTitle) {
            ::ChangeStageToWorld = originalWorld;
            ::ChangeStageToTitle = originalTitle;
        };
        // Death/quit/clear use the original title transition, including cleanup.
        // Restore both hooks before it runs so later ordinary play is unaffected.
        local leave = function() : (restore, originalTitle) {
            restore();
            originalTitle.call(getroottable());
        };
        ::ChangeStageToWorld = leave;
        ::ChangeStageToTitle = leave;
        try {
            // Original InitStage's fourth-character 's' convention suppresses
            // world-number intros for this mods/ path; no loader rewrite needed.
            InitStage(practiceMap);
            ::life = 99;
            PlayerStatus.life = 99;
            ::stageTimeStop = true;
            Fader2.FadeIn(1, 0, 0, 0);
        } catch (error) {
            restore();
            throw error;
        }
    }
});
