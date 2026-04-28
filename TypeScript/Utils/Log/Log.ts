import LogType from "./LogType";

/** 日志输出 */
class Log {
    
    /** 打印普通的日志 */
    public static Log(Type: LogType, Message: string): void {
        console.log(`[${LogType[Type]}] ${Message}`);
    }
    
    /** 打印警告日志 */
    public static Warring(Type: LogType, Message: string): void {
        console.warn(`[${LogType[Type]}] ${Message}`);
    }
    
    /** 打印错误日志 */
    public static Error(Type: LogType, Message: string): void {
        console.error(`[${LogType[Type]}] ${Message}`);
    }
}

export default Log;