import { flow, Instance, types } from 'mobx-state-tree';


const PhysioKitState = types
.model('HeartKitState', {
    appState: types.string,
})

export interface IPhysioKitState extends Instance<typeof PhysioKitState> {}


export const Root = types
.model('Root', {
    state: PhysioKitState
})
.actions(self => ({
    // fetchState: flow(function* () {
    //     const state = yield api.getState();
    //     if (state !== undefined) {
    //         self.state = state;
    //     }
    // }),
}))
.actions(self => ({
    backgroundRoutine: function() {
        console.log('Background done.');
    }
}))
.actions(self => ({
    afterCreate() {
        setInterval(self.backgroundRoutine, 2000);
    },
    beforeDestroy() {

    }
}))

export interface IRoot extends Instance<typeof Root> {}
