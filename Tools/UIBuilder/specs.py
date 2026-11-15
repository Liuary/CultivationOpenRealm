"""《观澜》测试套件的窗口规格。

每个窗口返回：
    {"asset_path": "/Game/UI/TestSuite/UI_TestXxx.UI_TestXxx",
     "parent": "/Script/CORUI.CORWindow" | "/Script/CORUI.CORFullScreen",
     "card": {...} | None,
     "nodes": [ ... ]}
坐标相对卡片左上角；card=None 时相对屏幕（1920x1080）。
"""

LAYERS = [("背景", "BACK", "BackGround", "100"),
          ("常规", "NORMAL", "Normal", "200"),
          ("顶层", "TOP", "Top", "300"),
          ("消息", "MESSAGE", "Message", "400"),
          ("调试", "DEBUG", "Debug", "500")]


def T(name, x, y, w, h, text, size, color, justify="Left", face="Regular", spacing=0,
      wrap=0, mode=None):
    node = {"kind": "text", "name": name, "x": x, "y": y, "w": w, "h": h,
            "text": text, "size": size, "color": color, "justify": justify,
            "face": face, "spacing": spacing}
    if wrap:
        node["wrap"] = wrap
    if mode:
        node["mode"] = mode
    return node


def R(name, x, y, w, h, color, fill=False, mode=None):
    node = {"kind": "rect", "name": name, "x": x, "y": y, "w": w, "h": h, "color": color}
    if fill:
        node["fill"] = True
    if mode:
        node["mode"] = mode
    return node


def B(name, x, y, w, h, label, size=16, variant="ghost", align="Center",
      color=None, border_color=None, disabled=False):
    node = {"kind": "button", "name": name, "x": x, "y": y, "w": w, "h": h,
            "label": label, "size": size, "variant": variant, "align": align}
    if color is not None:
        node["color"] = color
    if border_color is not None:
        node["border_color"] = border_color
    if disabled:
        node["disabled"] = True
    return node


def CI(name, x, y, w, h, color):
    return {"kind": "cor_image", "name": name, "x": x, "y": y, "w": w, "h": h, "color": color}


def P(name, x, y, w, h, color, percent=0.0):
    return {"kind": "progress", "name": name, "x": x, "y": y, "w": w, "h": h,
            "color": color, "percent": percent}


def title_block(C, cn, en, desc):
    return [
        T("Txt_Title", 40, 28, 420, 44, cn, 32, C["paper"]),
        T("Txt_Sub", 40, 76, 380, 14, en, 10, C["paper2"], spacing=2),
        T("Txt_Desc", 40, 100, 760, 16, desc, 11, C["paper3"]),
        R("Line_Header", 40, 128, 880, 1, C["line"]),
    ]


def hub(C):
    W, H = 1120, 640
    nodes = []
    nodes.append(T("Txt_Title", 48, 30, 420, 60, "测试台", 40, C["paper"]))
    nodes.append(T("Txt_Sub", 48, 94, 520, 16, "CORUI · UI TEST CONSOLE", 10, C["paper2"], spacing=2))
    nodes.append(R("Img_Seal", 1048, 40, 12, 12, C["ember"]))
    nodes.append(R("Line_Header", 48, 132, 1024, 1, C["line"]))

    nav = [("层级", "LAYERS", "Btn_Nav_Layer"),
           ("窗口栈", "STACK", "Btn_Nav_Stack"),
           ("数据", "DATA", "Btn_Nav_Data"),
           ("控件", "WIDGETS", "Btn_Nav_Widgets"),
           ("调试", "DEBUG", "Btn_Nav_Debug")]
    for i, (cn, en, name) in enumerate(nav):
        y = 168 + i * 64
        nodes.append(B(name, 48, y + 2, 240, 34, cn, size=22, variant="nav", align="Left"))
        nodes.append(T(name + "_En", 60, y + 36, 220, 12, en, 10, C["paper2"], spacing=2))
        nodes.append(R(name + "_Line", 48, y + 58, 240, 1, C["line"]))

    nodes.append(T("Txt_Layers", 340, 168, 80, 22, "层级", 16, C["paper"]))
    nodes.append(T("Txt_Layers_En", 386, 171, 120, 14, "LAYERS", 10, C["paper2"], spacing=2))
    for i, (cn, en, key, z) in enumerate(LAYERS):
        y = 200 + i * 56
        nodes.append(T("Txt_Layer_" + key + "_Cn", 340, y, 80, 20, cn, 18, C["paper"]))
        nodes.append(T("Txt_Layer_" + key + "_En", 386, y + 3, 140, 16, en, 10, C["paper2"], spacing=2))
        nodes.append(T("Txt_Layer_" + key + "_Count", 972, y, 100, 20, "0", 12, C["paper2"], justify="Right"))
        nodes.append(R("Line_Layer_" + key, 340, y + 38, 732, 1, C["line"]))

    nodes.append(T("Txt_Events", 340, 504, 80, 22, "事件", 16, C["paper"]))
    nodes.append(T("Txt_Events_En", 386, 507, 120, 14, "EVENTS", 10, C["paper2"], spacing=2))
    nodes.append(T("Txt_Event_0", 340, 528, 400, 16, "—", 11, C["paper3"]))
    nodes.append(T("Txt_Event_1", 340, 548, 400, 16, "—", 11, C["paper3"]))
    nodes.append(T("Txt_Event_2", 340, 568, 400, 16, "—", 11, C["paper3"]))

    nodes.append(B("Btn_SelfCheck", 620, 584, 116, 40, "自检", size=14))
    nodes.append(B("Btn_CloseAll", 748, 584, 156, 40, "关闭全部", size=14))
    nodes.append(B("Btn_Close", 916, 584, 120, 40, "收起", size=14))
    nodes.append(T("Txt_Hint", 48, 600, 420, 14, "开发期测试台 · 仅统计本套件推入的窗口", 10, C["paper3"]))

    return {
        "asset_path": "/Game/UI/TestSuite/UI_TestHubMain.UI_TestHubMain",
        "card": {"w": W, "h": H, "scrim": True, "scrim_color": C["scrim"]},
        "nodes": nodes,
    }


def layer(C):
    W, H = 960, 560
    nodes = title_block(C, "层级", "LAYERS · ZORDER", "验证五层容器、ZOrder 排序与跨层推入。")
    for i, (cn, en, key, z) in enumerate(LAYERS):
        y = 152 + i * 66
        nodes.append(T("Txt_" + key + "_Cn", 40, y, 80, 22, cn, 18, C["paper"]))
        nodes.append(T("Txt_" + key + "_En", 40, y + 26, 90, 12, en, 10, C["paper2"], spacing=2))
        nodes.append(T("Txt_" + key + "_Z", 150, y + 2, 50, 18, z, 12, C["paper2"]))
        nodes.append(T("Txt_Count_" + key, 206, y + 2, 140, 18, "0 个窗口", 12, C["paper3"]))
        nodes.append(B("Btn_Push_" + key, 760, y - 2, 120, 44, "推入", size=14))
        nodes.append(R("Line_" + key, 40, y + 54, 880, 1, C["line"]))
    nodes.append(B("Btn_AllClose", 636, 494, 140, 40, "全部收起", size=14))
    nodes.append(B("Btn_Back", 792, 494, 128, 40, "返回", size=14))
    nodes.append(T("Txt_Hint", 40, 506, 500, 14, "背景→背景层 · 常规→窗口栈 · 顶层→确认 · 消息→通知 · 调试→调试面板", 10, C["paper3"]))
    return {"asset_path": "/Game/UI/TestSuite/UI_TestLayer2.UI_TestLayer2",
            "card": {"w": W, "h": H, "scrim": True, "scrim_color": C["scrim"]},
            "nodes": nodes}


def stack(C):
    W, H = 960, 560
    nodes = title_block(C, "窗口栈", "STACK · PUSH / POP", "同名重推、只弹上方、覆层隐藏。")
    nodes.append(T("Txt_Depth", 40, 148, 300, 14, "栈深 STACK DEPTH", 10, C["paper2"], spacing=2))
    for i in range(5):
        y = 172 + i * 36
        nodes.append(R("Img_Rail_" + str(i), 40, y, 400, 30, C["panel2"]))
        nodes.append(T("Txt_Rail_" + str(i), 52, y + 7, 380, 18, "—", 12, C["paper2"]))
    nodes.append(B("Btn_Push", 500, 148, 220, 44, "推入卡片", size=14))
    nodes.append(B("Btn_Repush", 500, 200, 220, 44, "同名重推", size=14))
    nodes.append(B("Btn_PopTop", 500, 252, 220, 44, "弹出栈顶", size=14))
    nodes.append(B("Btn_PopBelow", 500, 304, 220, 44, "按名弹出", size=14))
    nodes.append(B("Btn_PopAll", 500, 356, 220, 44, "全部弹出", size=14))
    nodes.append(B("Btn_Cover", 500, 408, 220, 44, "覆层测试", size=14))
    nodes.append(T("Txt_LogTitle", 40, 388, 200, 14, "事件 EVENTS", 10, C["paper2"], spacing=2))
    nodes.append(T("Txt_Log_0", 40, 410, 430, 16, "—", 10, C["paper3"]))
    nodes.append(T("Txt_Log_1", 40, 428, 430, 16, "—", 10, C["paper3"]))
    nodes.append(T("Txt_Log_2", 40, 446, 430, 16, "—", 10, C["paper3"]))
    nodes.append(T("Txt_Hint", 500, 462, 420, 14, "覆层：以 Normal 层推入确认框，验证 HiddenUnder", 10, C["paper3"]))
    nodes.append(B("Btn_Back", 792, 494, 128, 40, "返回", size=14))
    return {"asset_path": "/Game/UI/TestSuite/UI_TestStack.UI_TestStack",
            "card": {"w": W, "h": H, "scrim": True, "scrim_color": C["scrim"]},
            "nodes": nodes}


def card(C):
    W, H = 380, 232
    nodes = [
        T("Txt_Card_Title", 24, 16, 160, 32, "卡片", 22, C["paper"]),
        T("Txt_Card_Sub", 24, 50, 120, 12, "CARD · PAGE", 10, C["paper2"], spacing=2),
        T("Txt_Card_Serial", 300, 18, 56, 20, "03", 12, C["paper2"], justify="Right"),
        R("Line_0", 24, 74, 332, 1, C["line"]),
        T("Txt_Card_Body", 24, 88, 332, 50, "测试卡片，验证窗口栈。", 12, C["paper2"], wrap=332),
        T("Txt_Card_Stats", 24, 140, 332, 14, "构造 1 · 入栈 1 · 实例 #—", 10, C["paper3"]),
        R("Line_1", 24, 160, 332, 1, C["line"]),
        B("Btn_Close", 24, 176, 120, 36, "关闭", size=13),
        B("Btn_Repush", 152, 176, 120, 36, "同名推入", size=13),
        T("Txt_Card_Hint", 286, 184, 70, 16, "CARD", 9, C["paper3"]),
    ]
    return {"asset_path": "/Game/UI/TestSuite/UI_TestCard2.UI_TestCard2",
            "card": {"w": W, "h": H, "scrim": True, "scrim_color": C["scrim"]},
            "nodes": nodes}


def data(C):
    W, H = 960, 560
    nodes = title_block(C, "数据", "DATA · SINGLE / POOL", "UISingle 传参、实例复用与同步/异步加载。")
    info = [("名称", "Txt_Data_Name", "示例"), ("序号", "Txt_Data_Serial", "07"),
            ("备注", "Txt_Data_Tone", "普通"), ("方式", "Txt_Data_Mode", "异步")]
    for i, (label, name, value) in enumerate(info):
        y = 156 + i * 52
        nodes.append(T("Txt_Label_" + name, 40, y, 100, 20, label, 11, C["paper3"]))
        nodes.append(T(name, 150, y - 2, 220, 24, value, 16, C["paper"]))
        nodes.append(R("Line_Row_" + str(i), 40, y + 38, 340, 1, C["line"]))
    nodes.append(B("Btn_Sync", 40, 382, 170, 44, "同步推入", size=14))
    nodes.append(B("Btn_Async", 226, 382, 170, 44, "异步推入", size=14))

    nodes.append(T("Txt_Lifecycle", 520, 152, 300, 14, "生命周期 LIFECYCLE", 10, C["paper2"], spacing=2))
    for i, (label, name) in enumerate([("Construct", "Txt_Count_Construct"), ("OnOpen", "Txt_Count_Open"),
                                       ("OnClose", "Txt_Count_Close"), ("Destruct", "Txt_Count_Destruct")]):
        y = 176 + i * 30
        nodes.append(T("Txt_Life_" + label, 520, y, 200, 18, label, 12, C["paper2"]))
        nodes.append(T(name, 860, y, 60, 18, "0", 12, C["paper"], justify="Right"))
    nodes.append(T("Txt_ReuseTitle", 520, 312, 300, 14, "实例复用 INSTANCE", 10, C["paper2"], spacing=2))
    nodes.append(T("Txt_Reuse", 520, 338, 300, 20, "复用 是", 14, C["paper"]))
    nodes.append(T("Txt_Instance", 520, 362, 300, 18, "实例 #—", 11, C["paper3"]))
    nodes.append(T("Txt_PayloadTitle", 520, 400, 300, 14, "当前载荷 PAYLOAD", 10, C["paper2"], spacing=2))
    nodes.append(T("Txt_Payload", 520, 424, 380, 40, "—", 11, C["paper3"], wrap=380))

    nodes.append(T("Txt_LogTitle", 40, 442, 200, 14, "事件 EVENTS", 10, C["paper2"], spacing=2))
    nodes.append(T("Txt_Log_0", 40, 464, 440, 16, "—", 10, C["paper3"]))
    nodes.append(T("Txt_Log_1", 40, 482, 440, 16, "—", 10, C["paper3"]))
    nodes.append(B("Btn_Back", 792, 494, 128, 40, "返回", size=14))
    return {"asset_path": "/Game/UI/TestSuite/UI_TestData.UI_TestData",
            "card": {"w": W, "h": H, "scrim": True, "scrim_color": C["scrim"]},
            "nodes": nodes}


def widgets(C):
    W, H = 960, 560
    nodes = title_block(C, "控件", "WIDGETS · COMPONENTS", "CORButton / CORImage / CORTextBlock / CORProgressBar。")
    nodes.append(T("Txt_Sec_Button", 40, 148, 220, 16, "按钮 BUTTON", 11, C["paper2"], spacing=1))
    nodes.append(T("Txt_Sec_Image", 500, 148, 220, 16, "图像 IMAGE", 11, C["paper2"], spacing=1))
    nodes.append(T("Txt_Sec_Text", 40, 330, 220, 16, "文本 TEXT", 11, C["paper2"], spacing=1))
    nodes.append(T("Txt_Sec_Progress", 500, 330, 240, 16, "进度 PROGRESS", 11, C["paper2"], spacing=1))

    nodes.append(B("Btn_Normal", 40, 176, 150, 44, "常态", size=14))
    nodes.append(B("Btn_Hover", 200, 176, 150, 44, "悬停", size=14))
    nodes.append(B("Btn_Pressed", 40, 230, 150, 44, "按下", size=14))
    nodes.append(B("Btn_Disabled", 200, 230, 150, 44, "禁用", size=14, disabled=True))

    nodes.append(R("Img_Placeholder", 500, 176, 380, 110, C["panel2"]))
    nodes.append(CI("Img_Async", 500, 176, 380, 110, C["panel2"]))
    nodes.append(T("Txt_ImageHint", 500, 214, 380, 24, "异步加载", 12, C["paper3"], justify="Center"))
    nodes.append(T("Txt_ImageMatch", 780, 262, 100, 14, "matchSize", 10, C["paper3"], justify="Right"))
    nodes.append(B("Btn_LoadImage", 500, 296, 120, 32, "加载图像", size=12))

    nodes.append(T("Txt_SampleTitle", 40, 358, 420, 36, "标题文本示例", 24, C["paper"]))
    nodes.append(T("Txt_SampleBody", 40, 400, 420, 24, "正文文本示例", 16, C["paper2"]))
    nodes.append(T("Txt_SampleCap", 40, 430, 420, 18, "注释文本示例", 12, C["paper3"]))

    nodes.append(P("Progress_Test", 500, 362, 300, 4, C["ember"], 0.62))
    nodes.append(T("Txt_Progress", 812, 353, 68, 20, "62%", 12, C["paper2"], justify="Right"))
    nodes.append(B("Btn_Replay", 500, 396, 100, 36, "重播", size=12))
    nodes.append(B("Btn_Back", 792, 494, 128, 40, "返回", size=14))
    return {"asset_path": "/Game/UI/TestSuite/UI_TestWidgets.UI_TestWidgets",
            "card": {"w": W, "h": H, "scrim": True, "scrim_color": C["scrim"]},
            "nodes": nodes}


def confirm(C):
    W, H = 560, 280
    nodes = [
        T("Txt_Title", 32, 24, 160, 30, "确认", 22, C["paper"]),
        T("Txt_Sub", 32, 54, 140, 12, "CONFIRM", 10, C["paper2"], spacing=2),
        R("Line_0", 32, 78, 496, 1, C["line"]),
        T("Txt_Confirm_Body", 32, 96, 496, 64, "是否清空本次会话的全部窗口？", 14, C["paper"], wrap=496),
        R("Line_1", 32, 176, 496, 1, C["line"]),
        B("Btn_Cancel", 216, 206, 150, 40, "取消", size=14),
        B("Btn_Confirm", 382, 206, 146, 40, "确认", size=14, border_color=C["ember"]),
    ]
    return {"asset_path": "/Game/UI/TestSuite/UI_TestConfirm.UI_TestConfirm",
            "parent": "/Script/CORUI.CORFullScreen",
            "card": {"w": W, "h": H, "scrim": True, "scrim_color": C["scrim"]},
            "nodes": nodes}


def toast(C):
    W, H = 360, 88
    nodes = [
        R("Img_Bar", 1, 1, 2, H - 2, C["ember"]),
        T("Txt_Toast_Title", 18, 12, 320, 20, "通知", 14, C["paper"]),
        T("Txt_Toast_Body", 18, 34, 324, 40, "消息层窗口 · 3.2 秒后自动收起。", 11, C["paper2"], wrap=324),
    ]
    return {"asset_path": "/Game/UI/TestSuite/UI_TestToast.UI_TestToast",
            "card": {"w": W, "h": H, "anchor": "topright", "mx": 24, "my": 24},
            "nodes": nodes}


def debug(C):
    nodes = [
        R("Img_PanelBorder", 40, 40, 380, 540, C["line"]),
        R("Img_Panel", 41, 41, 378, 538, C["ink0"]),
        T("Txt_Title", 60, 56, 160, 28, "调试", 20, C["paper"]),
        T("Txt_Sub", 60, 86, 200, 12, "DEBUG · OVERLAY", 10, C["paper2"], spacing=2),
        T("Txt_Fps", 60, 112, 340, 18, "FPS 60 · 帧 16.6 ms", 12, C["paper2"]),
        T("Txt_LayerTitle", 60, 144, 200, 14, "层级 LAYERS", 10, C["paper2"], spacing=2),
    ]
    for i, (cn, en, key, z) in enumerate(LAYERS):
        nodes.append(T("Txt_Count_" + key, 60, 166 + i * 22, 320, 18, cn + "    0", 11, C["paper3"]))
    nodes.append(T("Txt_EventTitle", 60, 296, 200, 14, "事件 EVENTS", 10, C["paper2"], spacing=2))
    for i in range(8):
        nodes.append(T("Txt_Log_" + str(i), 60, 318 + i * 18, 340, 16, "—", 10, C["paper3"]))
    nodes.append(B("Btn_Clear", 60, 536, 120, 32, "清空", size=12))
    nodes.append(B("Btn_Hide", 192, 536, 120, 32, "收起", size=12))
    return {"asset_path": "/Game/UI/TestSuite/UI_TestDebug.UI_TestDebug",
            "card": None,
            "nodes": nodes}


def backdrop(C):
    nodes = [
        R("Img_Dim", 0, 0, 0, 0, C["scrim"], fill=True),
        T("Txt_Tag", 48, -24, 400, 16, "背景层 · ZOrder 100", 10, C["paper3"],
          mode="bottomleft"),
    ]
    return {"asset_path": "/Game/UI/TestSuite/UI_TestBackdrop.UI_TestBackdrop",
            "card": None,
            "nodes": nodes}


WINDOWS = {
    "hub": hub,
    "layer": layer,
    "stack": stack,
    "card": card,
    "data": data,
    "widgets": widgets,
    "confirm": confirm,
    "toast": toast,
    "debug": debug,
    "backdrop": backdrop,
}
