/// <reference path="puerts.d.ts" />
declare module "ue" {
    import {$Ref, $Nullable} from "puerts"

    import * as cpp from "cpp"

    import * as UE from "ue"

// __TYPE_DECL_START: 7F1F6BAE447E1BCB2F775FA8582F08E1
    namespace Game.UI.GameStart.UI_GameStart {
        class UI_GameStart_C extends UE.CORFullScreen {
            constructor(Outer?: Object, Name?: string, ObjectFlags?: number);
            Ani_FadeOut: UE.WidgetAnimation;
            Ani_FadeIn: UE.WidgetAnimation;
            Img_Start: UE.CORImage;
            Construct() : void;
            Destruct() : void;
            OnClose() : void;
            OnHide() : void;
            OnOpen() : void;
            OnShow() : void;
            static StaticClass(): Class;
            static Find(OrigInName: string, Outer?: Object): UI_GameStart_C;
            static Load(InName: string): UI_GameStart_C;
        
            __tid_UI_GameStart_C_0__: boolean;
        }
        
    }

// __TYPE_DECL_END
// __TYPE_DECL_START: A00831854E60DE0442254583165F1EEE
    namespace Game.UI.GameLoading.UI_GameLoading {
        class UI_GameLoading_C extends UE.CORFullScreen {
            constructor(Outer?: Object, Name?: string, ObjectFlags?: number);
            Progress_Loading: UE.CORProgressBar;
            Switcher_Load: UE.WidgetSwitcher;
            Txt_Loaded: UE.CORTextBlock;
            Txt_Loading: UE.CORTextBlock;
            Txt_Progress: UE.CORTextBlock;
            static StaticClass(): Class;
            static Find(OrigInName: string, Outer?: Object): UI_GameLoading_C;
            static Load(InName: string): UI_GameLoading_C;
        
            __tid_UI_GameLoading_C_0__: boolean;
        }
        
    }

// __TYPE_DECL_END
}
