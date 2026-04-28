import * as UE from 'ue'
import {argv} from 'puerts';

import './PuertsImportList';

import GameManager from "./Manager/GameManager";
import Log from "./Utils/Log/Log";
import LogType from "./Utils/Log/LogType";

let World: UE.World = (argv.getByName("GameInstance") as UE.GameInstance).GetWorld();

let GameInstance: UE.GameInstance = argv.getByName("GameInstance") as UE.GameInstance;

class CorStart {
    constructor() {
        Log.Log(LogType.Game, `[CorStart][Constructor] Typescript is constructing.`);
    }

    private static _Instance: CorStart = null;
    public static get Instance(): CorStart {
        if (this._Instance == null) 
        {
            this._Instance = new CorStart();
        }
        return this._Instance;
    }

    Start() {
        Log.Log(LogType.Game, `[CorStart][Start] Typescript is started.`);
        GameManager.Instance.BindCppEvent();
    }

    GetWorld(): UE.World {
        return World;
    }

    GetGameInstance(): UE.GameInstance {

        return GameInstance;

    }
}

export default CorStart;

CorStart.Instance.Start();