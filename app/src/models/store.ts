import React from "react";
import { unprotect } from "mobx-state-tree";
import { IRoot, Root } from "./root";

const root = Root.create({
    state: {
        appState: 'IDLE',
    }
});

const store = {
    root
};

unprotect(root);

export const StoreContext = React.createContext(store);
export const useStore = () => React.useContext(StoreContext);
export default store;

export type IStore = {
    root: IRoot
};
