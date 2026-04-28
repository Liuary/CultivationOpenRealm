class GameManagerBase {
    constructor() {
        this.Init = this.Init.bind(this);
        this.Deinit = this.Deinit.bind(this);
    }
    
    /** 初始化管理器 */
    public Init(): void {
    }
    
    /** 反初始化管理器 */
    public Deinit(): void {
        
    }
}

export default GameManagerBase; 