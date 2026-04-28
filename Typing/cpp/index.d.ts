declare module "cpp" {
    import * as UE from "ue"
    import * as cpp from "cpp"
    import {$Ref, $Nullable, cstring} from "puerts"

    class CORSoftObjectPath {
        constructor(p0: string);
        constructor();
        static MakeSoftObjectPath(p0: string) :CORSoftObjectPath;
        SetPath(p0: string) :void;
        GetAsset() :UE.Object;
        GetPath() :string;
        IsValid() :boolean;
        Load() :UE.Object;
    }

    abstract class FFloat16Color {
    }

    abstract class FPuertsEditorModule {
        static SetCmdCallback(p0: (p0:string, p1:string) => void) :void;
    }

    class WeakObjectWrapper {
        constructor(p0: UE.Object);
        constructor();
        static MakeWeakObjectPtr(p0: UE.Object) :WeakObjectWrapper;
        Set(p0: UE.Object) :void;
        Get() :UE.Object;
        IsValid() :boolean;
        Reset() :void;
    }

}
