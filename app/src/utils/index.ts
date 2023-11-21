

export function delay<Type>(t: number, val?: Type): Promise<Type|undefined> {
    return new Promise(function(resolve) {
        setTimeout(function() {
            resolve(val);
        }, t);
    });
}
