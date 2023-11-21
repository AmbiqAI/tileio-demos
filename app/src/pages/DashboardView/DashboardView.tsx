
import { observer } from "mobx-react-lite";
import { useStore } from "../../models/store";

const DashboardView = () => {
  const { root: { state }} = useStore();
  return (
    <div>
        Test
    </div>
  );
}

export default observer(DashboardView);
