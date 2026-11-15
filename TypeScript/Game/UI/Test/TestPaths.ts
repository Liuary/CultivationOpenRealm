/** 《观澜》测试套件的资产路径与窗口命名。 */
import * as UE from 'ue';

const ROOT = '/Game/UI/TestSuite';

export const TestClassPaths = {
    Hub: `${ROOT}/UI_TestHubMain.UI_TestHubMain_C`,
    Layer: `${ROOT}/UI_TestLayer2.UI_TestLayer2_C`,
    Stack: `${ROOT}/UI_TestStack.UI_TestStack_C`,
    Card: `${ROOT}/UI_TestCard2.UI_TestCard2_C`,
    Data: `${ROOT}/UI_TestData.UI_TestData_C`,
    Widgets: `${ROOT}/UI_TestWidgets.UI_TestWidgets_C`,
    Confirm: `${ROOT}/UI_TestConfirm.UI_TestConfirm_C`,
    Toast: `${ROOT}/UI_TestToast.UI_TestToast_C`,
    Debug: `${ROOT}/UI_TestDebug.UI_TestDebug_C`,
    Backdrop: `${ROOT}/UI_TestBackdrop.UI_TestBackdrop_C`,
};

/** PushUI 用的软类路径（与现有 GameStart/GameLoading 保持同一写法）。 */
export function PushPath(ClassPath: string): string {
    return `/Script/UMGEditor.WidgetBlueprint'${ClassPath}'`;
}

export const TestWindowNames = {
    Hub: 'UI_TestHub',
    Layer: 'UI_TestLayer',
    Stack: 'UI_TestStack',
    Data: 'UI_TestData',
    Widgets: 'UI_TestWidgets',
    Confirm: 'UI_TestConfirm',
    Toast: 'UI_TestToast',
    Debug: 'UI_TestDebug',
    Backdrop: 'UI_TestBackdrop',
    CardPrefix: 'UI_TestCard',
};

/** 层级数值与 C++ EUILayerType 一致。 */
export const TestLayer = {
    BackGround: UE.EUILayerType.BackGround,
    Normal: UE.EUILayerType.Normal,
    Top: UE.EUILayerType.Top,
    Message: UE.EUILayerType.Message,
    Debug: UE.EUILayerType.Debug,
};

export function LayerKey(Layer: number): string {
    switch (Number(Layer)) {
        case 1: return 'BackGround';
        case 2: return 'Normal';
        case 3: return 'Top';
        case 4: return 'Message';
        case 5: return 'Debug';
        default: return 'None';
    }
}
