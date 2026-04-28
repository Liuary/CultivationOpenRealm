/**
 * 提取一个类中的所有非函数属性
 */
export type PureData<T> = {
    [P in keyof T as T[P] extends Function ? never : P]: T[P];
}