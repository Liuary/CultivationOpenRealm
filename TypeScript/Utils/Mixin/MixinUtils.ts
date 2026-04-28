export function ApplyMixins(DerivedCtor: any, constructors: any[]) {
    constructors.forEach((BaseCtor) => {
        Object.getOwnPropertyNames(BaseCtor.prototype).forEach((name)=>{
            // 不覆盖构造函数和子类已经重写的成员
            if (name != 'constructor' && !DerivedCtor.prototype.hasOwnProperty(name)){
                Object.defineProperty(
                    DerivedCtor.prototype,
                    name,
                    Object.getOwnPropertyDescriptor(BaseCtor.prototype, name) || Object.create(null)
                )
            }
        });
    });
}