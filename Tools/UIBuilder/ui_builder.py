"""UGM 构建器：把声明式窗口规格转成 UE MCP 调用。

用法：
    python Tools/UIBuilder/ui_builder.py hub
    python Tools/UIBuilder/ui_builder.py hub --keep   # 不删除已存在资产
"""

import argparse
import json
import sys
import urllib.request

MCP_URL = "http://127.0.0.1:8000/mcp"

# ghost 按钮的描边宽度（UMG 像素）：1 在 1080p + DPI 0.72 下几乎不可见，5 太粗；取 2。
BUTTON_BORDER = 2

FONT_REGULAR = "/Game/Font/LXGWWenKai/Font_LXGWWenKai.Font_LXGWWenKai"
BLANK = "/Game/Asset/UI/Common/Blank.Blank"

COLORS = {
    "ink0": (0.039, 0.035, 0.031, 1.0),
    "panel": (0.071, 0.063, 0.055, 0.95),
    "panel2": (0.102, 0.090, 0.078, 0.95),
    "paper": (0.929, 0.902, 0.855, 1.0),
    "paper2": (0.663, 0.627, 0.576, 1.0),
    "paper3": (0.431, 0.404, 0.365, 1.0),
    "ember": (0.784, 0.333, 0.184, 1.0),
    "line": (0.929, 0.902, 0.855, 0.12),
    "line_btn": (0.929, 0.902, 0.855, 0.32),
    "scrim": (0.039, 0.035, 0.031, 0.72),
}


class McpClient:
    def __init__(self, url=MCP_URL):
        self.url = url
        self.session_id = None
        self.next_id = 1
        self.initialize()

    def _post(self, payload):
        data = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            self.url,
            data=data,
            headers={
                "Content-Type": "application/json",
                "Accept": "application/json, text/event-stream",
                **({"Mcp-Session-Id": self.session_id} if self.session_id else {}),
            },
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=180) as resp:
            body = resp.read().decode("utf-8")
            sid = resp.headers.get("Mcp-Session-Id")
            if sid:
                self.session_id = sid
        if body.startswith("data:"):
            body = body[5:].strip()
        if not body.strip():
            return {}
        return json.loads(body)

    def initialize(self):
        self._post({
            "jsonrpc": "2.0", "id": self.next_id, "method": "initialize",
            "params": {"protocolVersion": "2025-03-26", "capabilities": {},
                       "clientInfo": {"name": "corui-ui-builder", "version": "1.0"}},
        })
        self.next_id += 1
        self._post({"jsonrpc": "2.0", "method": "notifications/initialized"})

    def call_tool(self, toolset, tool, arguments):
        payload = {
            "jsonrpc": "2.0", "id": self.next_id, "method": "tools/call",
            "params": {"name": "call_tool",
                       "arguments": {"toolset_name": toolset, "tool_name": tool,
                                     "arguments": arguments}},
        }
        self.next_id += 1
        resp = self._post(payload)
        if "error" in resp:
            raise RuntimeError(resp["error"])
        content = resp.get("result", {}).get("content", [])
        text = content[0].get("text", "") if content else ""
        if resp.get("result", {}).get("isError"):
            raise RuntimeError(text)
        return text


def build_editor_script(spec):
    nodes_json = json.dumps(spec["nodes"], ensure_ascii=False)
    card = json.dumps(spec.get("card"))
    return f"""
import json
BP = {json.dumps(spec["asset_path"])}
PARENT = {json.dumps(spec.get("parent", "/Script/CORUI.CORWindow"))}
FONT = {json.dumps(FONT_REGULAR)}
BLANK = {json.dumps(BLANK)}
CARD = json.loads({json.dumps(card)})
NODES = json.loads({json.dumps(nodes_json)})

def umg(tool, args):
    return execute_tool("UMGToolSet.UMGToolSet." + tool, json.dumps(args))

def obj_set(inst, values):
    return execute_tool("editor_toolset.toolsets.object.ObjectTools.set_properties",
        json.dumps({{"instance": {{"refPath": inst}}, "values": json.dumps(values)}}))

def brush(rgba):
    return {{
        "tintColor": {{"specifiedColor": {{"r": rgba[0], "g": rgba[1], "b": rgba[2], "a": rgba[3]}},
                      "colorUseRule": "UseColor_Specified"}},
        "drawAs": "Image", "tiling": "NoTile", "mirroring": "NoMirror", "imageType": "FullColor",
        "imageSize": {{"x": 0, "y": 0}},
        "margin": {{"left": 0, "top": 0, "right": 0, "bottom": 0}},
        "resourceObject": {{"refPath": BLANK}},
    }}

def slate_color(rgba):
    return {{"specifiedColor": {{"r": rgba[0], "g": rgba[1], "b": rgba[2], "a": rgba[3]}},
             "colorUseRule": "UseColor_Specified"}}

def font(size, face="Regular", spacing=0):
    return {{"fontObject": {{"refPath": FONT}}, "typefaceFontName": face,
             "size": size, "letterSpacing": spacing}}

def add(cls, name, parent, x, y, w, h, mode="tl", set_layout=True):
    kw = {{"widgetBlueprint": {{"refPath": BP}}, "widgetClass": {{"refPath": cls}},
           "widgetDisplayName": name}}
    if parent:
        kw["parentWidget"] = {{"refPath": parent}}
    info = umg("AddWidget", kw).get("returnValue")
    if not info:
        raise RuntimeError("AddWidget failed: " + name)
    widget = info["widget"]["refPath"]
    slot = info["slot"]["refPath"] if info.get("slot") else None
    layout = {{
        "offsets": {{"left": x, "top": y, "right": w, "bottom": h}},
        "anchors": {{"minimum": {{"x": 0, "y": 0}}, "maximum": {{"x": 0, "y": 0}}}},
        "alignment": {{"x": 0, "y": 0}},
    }}
    if mode == "fill":
        layout = {{"offsets": {{"left": 0, "top": 0, "right": 0, "bottom": 0}},
                   "anchors": {{"minimum": {{"x": 0, "y": 0}}, "maximum": {{"x": 1, "y": 1}}}},
                   "alignment": {{"x": 0, "y": 0}}}}
    elif mode == "center":
        layout = {{"offsets": {{"left": 0, "top": 0, "right": w, "bottom": h}},
                   "anchors": {{"minimum": {{"x": 0.5, "y": 0.5}}, "maximum": {{"x": 0.5, "y": 0.5}}}},
                   "alignment": {{"x": 0.5, "y": 0.5}}}}
    elif mode == "topright":
        layout = {{"offsets": {{"left": x, "top": y, "right": w, "bottom": h}},
                   "anchors": {{"minimum": {{"x": 1, "y": 0}}, "maximum": {{"x": 1, "y": 0}}}},
                   "alignment": {{"x": 1, "y": 0}}}}
    elif mode == "bottomleft":
        layout = {{"offsets": {{"left": x, "top": y, "right": w, "bottom": h}},
                   "anchors": {{"minimum": {{"x": 0, "y": 1}}, "maximum": {{"x": 0, "y": 1}}}},
                   "alignment": {{"x": 0, "y": 1}}}}
    if slot and set_layout:
        obj_set(slot, {{"layoutData": layout, "bAutoSize": False}})
    umg("ToggleWidgetAsVariable", {{"widgetBlueprint": {{"refPath": BP}},
                                    "widget": {{"refPath": widget}}, "bIsVariable": True}})
    return widget, slot

def add_rect(node, parent):
    mode = "fill" if node.get("fill") else node.get("mode", "tl")
    w, _ = add("/Script/UMG.Image", node["name"], parent, node["x"], node["y"],
               node["w"], node["h"], mode)
    obj_set(w, {{"brush": brush(node["color"])}})
    return w

def add_text(node, parent):
    mode = node.get("mode", "tl")
    w, _ = add("/Script/UMG.TextBlock", node["name"], parent, node["x"], node["y"],
               node["w"], node["h"], mode)
    obj_set(w, {{"text": node["text"], "font": font(node["size"], node.get("face", "Regular"), node.get("spacing", 0)),
                 "colorAndOpacity": slate_color(node["color"]),
                 "justification": node.get("justify", "Left")}})
    if node.get("wrap"):
        obj_set(w, {{"autoWrapText": True, "wrapTextAt": node["wrap"]}})
    return w

def add_cor_image(node, parent):
    w, _ = add("/Script/CORUI.CORImage", node["name"], parent, node["x"], node["y"],
               node["w"], node["h"])
    obj_set(w, {{"brush": brush(node.get("color", (0.102, 0.09, 0.078, 1.0)))}})
    return w

def add_progress(node, parent):
    w, _ = add("/Script/CORUI.CORProgressBar", node["name"], parent, node["x"], node["y"],
               node["w"], node["h"])
    obj_set(w, {{"percent": node.get("percent", 0.0),
                 "fillColorAndOpacity": {{"r": node["color"][0], "g": node["color"][1],
                                         "b": node["color"][2], "a": node["color"][3]}}}})
    return w

BUTTON_STYLES = {{
    "ghost": {{
        "normal": (0.071, 0.063, 0.055, 1.0), "hovered": (0.13, 0.115, 0.10, 1.0),
        "pressed": (0.784, 0.333, 0.184, 0.18), "disabled": (0.071, 0.063, 0.055, 0.35),
    }},
    "nav": {{
        "normal": (0.0, 0.0, 0.0, 0.0), "hovered": (0.929, 0.902, 0.855, 0.05),
        "pressed": (0.784, 0.333, 0.184, 0.10), "disabled": (0.0, 0.0, 0.0, 0.0),
    }},
}}

def add_button(node, parent):
    variant = node.get("variant", "ghost")
    x, y, w, h = node["x"], node["y"], node["w"], node["h"]
    if variant == "ghost":
        add_rect({{"name": node["name"] + "_Border", "x": x, "y": y, "w": w, "h": h,
                  "color": node.get("border_color", (0.929, 0.902, 0.855, 0.70))}}, parent)
        B = {BUTTON_BORDER}
        x, y, w, h = x + B, y + B, w - 2 * B, h - 2 * B
    btn, slot = add("/Script/UMG.Button", node["name"], parent, x, y, w, h)
    colors = BUTTON_STYLES.get(variant, BUTTON_STYLES["ghost"])
    style = {{k: brush(colors[k]) for k in ("normal", "hovered", "pressed", "disabled")}}
    style["normalPadding"] = {{"left": 0, "top": 0, "right": 0, "bottom": 0}}
    style["pressedPadding"] = {{"left": 0, "top": 0, "right": 0, "bottom": 0}}
    obj_set(btn, {{"widgetStyle": style}})
    if node.get("disabled"):
        obj_set(btn, {{"bIsEnabled": False}})
    txt, txtslot = add("/Script/UMG.TextBlock", node["name"] + "_Label", btn,
                       0, 0, w, h, set_layout=False)
    align = node.get("align", "Center")
    if align == "Left":
        obj_set(txt, {{"text": node["label"], "font": font(node.get("size", 16), node.get("face", "Regular")),
                      "colorAndOpacity": slate_color(node.get("color", (0.929, 0.902, 0.855, 1.0))),
                      "justification": "Left"}})
        obj_set(txtslot, {{"padding": {{"left": 12, "top": 0, "right": 0, "bottom": 0}},
                           "horizontalAlignment": "HAlign_Fill", "verticalAlignment": "VAlign_Center"}})
    else:
        obj_set(txt, {{"text": node["label"], "font": font(node.get("size", 16), node.get("face", "Regular")),
                      "colorAndOpacity": slate_color(node.get("color", (0.929, 0.902, 0.855, 1.0))),
                      "justification": "Center"}})
        obj_set(txtslot, {{"padding": {{"left": 0, "top": 0, "right": 0, "bottom": 0}},
                           "horizontalAlignment": "HAlign_Center", "verticalAlignment": "VAlign_Center"}})
    return btn

def run():
    bp_ref = {{"refPath": BP}}
    info = None
    try:
        info = umg("GetWidgets", {{"widgetBlueprint": bp_ref}})["returnValue"]
    except Exception:
        info = None
    if info and info.get("widgets"):
        widgets = info["widgets"]
        root = widgets[0]["widget"]["refPath"]
        for w in widgets:
            parent = w.get("parent")
            if isinstance(parent, dict) and parent.get("refPath") == root:
                umg("RemoveWidget", {{"widgetBlueprint": bp_ref, "widget": w["widget"]}})
    else:
        bp = umg("CreateWidgetBlueprint", {{"folderPath": BP.rsplit("/", 1)[0],
                                           "assetName": BP.rsplit("/", 1)[1],
                                           "parentClass": {{"refPath": PARENT}}}})["returnValue"]
        root_info = umg("AddWidget", {{"widgetBlueprint": bp, "widgetClass": {{"refPath": "/Script/UMG.CanvasPanel"}},
                                      "widgetDisplayName": "Canvas_Root"}})["returnValue"]
        root = root_info["widget"]["refPath"]
    if CARD:
        if CARD.get("scrim"):
            scrim, _ = add("/Script/UMG.Image", "Img_Scrim", root, 0, 0, 0, 0, "fill")
            obj_set(scrim, {{"brush": brush(CARD["scrim_color"])}})
        if CARD.get("anchor") == "topright":
            card, _ = add("/Script/UMG.CanvasPanel", "Canvas_Card", root,
                          -CARD.get("mx", 24), CARD.get("my", 24),
                          CARD["w"], CARD["h"], "topright")
        else:
            card, _ = add("/Script/UMG.CanvasPanel", "Canvas_Card", root, 0, 0,
                          CARD["w"], CARD["h"], "center")
        add_rect({{"name": "Img_CardBorder", "x": 0, "y": 0, "w": CARD["w"], "h": CARD["h"],
                  "color": (0.929, 0.902, 0.855, 0.22)}}, card)
        add_rect({{"name": "Img_CardPanel", "x": 2, "y": 2, "w": CARD["w"] - 4, "h": CARD["h"] - 4,
                  "color": (0.071, 0.063, 0.055, 1.0)}}, card)
        target = card
    else:
        target = root
    counts = {{"rect": 0, "text": 0, "button": 0}}
    for node in NODES:
        kind = node["kind"]
        if kind == "rect":
            add_rect(node, target); counts["rect"] += 1
        elif kind == "text":
            add_text(node, target); counts["text"] += 1
        elif kind == "button":
            add_button(node, target); counts["button"] += 1
        elif kind == "cor_image":
            add_cor_image(node, target); counts["rect"] += 1
        elif kind == "progress":
            add_progress(node, target); counts["rect"] += 1
        else:
            raise RuntimeError("unknown node kind: " + kind)
    comp = umg("CompileWidgetBlueprint", {{"widgetBlueprint": bp_ref}})
    save = execute_tool("editor_toolset.toolsets.asset.AssetTools.save_assets",
                        json.dumps({{"asset_paths": [BP]}}))
    return {{"asset": BP, "widgets": counts, "compiled": comp.get("returnValue"),
             "saved": save.get("returnValue")}}
"""


def build(window_key, keep=False):
    from specs import WINDOWS
    spec = WINDOWS[window_key](COLORS)
    client = McpClient()
    script = build_editor_script(scale_spec(spec))
    result = client.call_tool(
        "editor_toolset.toolsets.programmatic.ProgrammaticToolset",
        "execute_tool_script",
        {"script": script},
    )
    if not keep:
        pass
    print(result)
    return result


SCALE = 1.25


def scale_spec(spec):
    """把设计稿坐标整体放大，抵消 1080p 下的 DPI 缩放（约 0.67~0.72）。"""
    spec = json.loads(json.dumps(spec))

    def S(Value):
        return round(Value * SCALE)

    card = spec.get("card")
    if card:
        card["w"] = S(card["w"])
        card["h"] = S(card["h"])
        if "mx" in card:
            card["mx"] = S(card["mx"])
        if "my" in card:
            card["my"] = S(card["my"])
    for Node in spec["nodes"]:
        for Key in ("x", "y", "w", "h"):
            if Key in Node:
                Node[Key] = S(Node[Key])
        if "size" in Node:
            Node["size"] = S(Node["size"])
        if Node.get("wrap"):
            Node["wrap"] = S(Node["wrap"])
    return spec


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("window")
    parser.add_argument("--keep", action="store_true")
    args = parser.parse_args()
    sys.path.insert(0, __file__.rsplit("\\", 1)[0] if "\\" in __file__ else __file__.rsplit("/", 1)[0])
    build(args.window, args.keep)


if __name__ == "__main__":
    main()
