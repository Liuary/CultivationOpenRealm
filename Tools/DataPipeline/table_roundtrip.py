# -*- coding: utf-8 -*-
"""数据表导入/导出往返验证脚本（编辑器 Python，仅用于验证，不是运行时资产）。

验证内容：
    1. 读取 /Game/Data/GameStart/DT_GameStart（只读，不保存任何资产）。
    2. ExportTableToJson / ExportTableToCsv 导出到内存字符串。
    3. CreateTableFromTemplate 新建内存表（复制 RowStruct），ImportTableFromJson 导入。
    4. 另一张内存表 ImportTableFromCsv 导入。
    5. 打印一行机器可读结果并退出编辑器。

用法：
    UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="Tools/DataPipeline/table_roundtrip.py" -unattended -nosplash -nopause -noP4

约束：不调用 save_asset / save_packages，磁盘资产保持原样。
"""

import sys
import unreal

ASSET_PATH = "/Game/Data/GameStart/DT_GameStart"
RESULT_FMT = "[table-roundtrip] original=%d json_import=%s json_rows=%d csv_import=%s csv_rows=%d"


def _quit():
    try:
        unreal.SystemLibrary.execute_console_command(None, "Quit")
    except Exception as exc:  # 编辑器可能已经开始关闭
        print("ROUNDTRIP_FAIL quit command failed: %s" % exc, flush=True)


def _error(problems, message):
    problems.append(message)
    unreal.log_error("ROUNDTRIP_FAIL " + message)


def _row_names(table):
    try:
        return [str(Name) for Name in table.get_row_names()]
    except Exception as exc:
        unreal.log_error("ROUNDTRIP_FAIL get_row_names raised: %s" % exc)
        return None


def main():
    problems = []
    original_rows = -1
    json_import_ok = False
    json_rows = -1
    csv_import_ok = False
    csv_rows = -1

    table = None
    try:
        table = unreal.load_asset(ASSET_PATH)
    except Exception as exc:
        _error(problems, "load_asset('%s') raised: %s" % (ASSET_PATH, exc))

    if not table:
        _error(problems, "asset not found: %s" % ASSET_PATH)
        print(RESULT_FMT % (original_rows, json_import_ok, json_rows, csv_import_ok, csv_rows), flush=True)
        _quit()
        return

    original_names = _row_names(table)
    if original_names is None:
        _error(problems, "cannot read original row names")
        original_names = []
    original_rows = len(original_names)
    unreal.log("table-roundtrip: original rows = %d" % original_rows)

    json_text = None
    csv_text = None
    try:
        json_text = unreal.CORDataLibrary.export_table_to_json(table)
    except Exception as exc:
        _error(problems, "export_table_to_json raised: %s" % exc)

    try:
        csv_text = unreal.CORDataLibrary.export_table_to_csv(table)
    except Exception as exc:
        _error(problems, "export_table_to_csv raised: %s" % exc)

    if not json_text:
        _error(problems, "export_table_to_json returned empty string")
    if not csv_text:
        _error(problems, "export_table_to_csv returned empty string")

    # JSON 导入（独立内存表）
    if json_text:
        target = None
        try:
            target = unreal.CORDataLibrary.create_table_from_template(table, "DT_RoundTripJson")
        except Exception as exc:
            _error(problems, "create_table_from_template(json) raised: %s" % exc)
        if not target:
            _error(problems, "create_table_from_template(json) returned None")
        else:
            try:
                json_import_ok = bool(unreal.CORDataLibrary.import_table_from_json(target, json_text))
            except Exception as exc:
                _error(problems, "import_table_from_json raised: %s" % exc)
            json_names = _row_names(target)
            json_rows = -1 if json_names is None else len(json_names)
            if not json_import_ok:
                _error(problems, "import_table_from_json returned False")
            elif json_names is None:
                _error(problems, "JSON import: cannot read row names")
            elif sorted(json_names) != sorted(original_names):
                _error(problems, "JSON row-name mismatch: original=%s json=%s" % (sorted(original_names), sorted(json_names)))

    # CSV 导入（独立内存表）
    if csv_text:
        target = None
        try:
            target = unreal.CORDataLibrary.create_table_from_template(table, "DT_RoundTripCsv")
        except Exception as exc:
            _error(problems, "create_table_from_template(csv) raised: %s" % exc)
        if not target:
            _error(problems, "create_table_from_template(csv) returned None")
        else:
            try:
                csv_import_ok = bool(unreal.CORDataLibrary.import_table_from_csv(target, csv_text))
            except Exception as exc:
                _error(problems, "import_table_from_csv raised: %s" % exc)
            csv_names = _row_names(target)
            csv_rows = -1 if csv_names is None else len(csv_names)
            if not csv_import_ok:
                _error(problems, "import_table_from_csv returned False")
            elif csv_names is None:
                _error(problems, "CSV import: cannot read row names")
            elif sorted(csv_names) != sorted(original_names):
                _error(problems, "CSV row-name mismatch: original=%s csv=%s" % (sorted(original_names), sorted(csv_names)))

    print(RESULT_FMT % (original_rows, json_import_ok, json_rows, csv_import_ok, csv_rows), flush=True)
    if problems:
        print("ROUNDTRIP_FAIL problems=%d" % len(problems), flush=True)
    else:
        unreal.log("table round-trip succeeded")

    _quit()

    # 有问题时返回非零退出码，便于 CI 直接判定（编辑器已请求退出）
    if problems:
        sys.exit(1)


main()

