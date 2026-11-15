import UISingleType from '../../../Utils/UISingle/UISingleType';

/** 测试套件的 UISingle 键位（复用通用键，不扩展 UISingleType）。 */
enum UI_TestSingleType {
    Title = UISingleType.Name,
    Serial = UISingleType.Id,
    Body = UISingleType.List,
    Tone = UISingleType.Flag_0,
    Mode = UISingleType.Flag_1,
    Layer = UISingleType.Flag_2,
    Index = UISingleType.Flag_3,
}

export default UI_TestSingleType;
