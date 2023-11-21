import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';
import "typeface-roboto"
import reportWebVitals from './reportWebVitals';
import { observer } from 'mobx-react-lite';
import { CssBaseline, ThemeProvider } from '@mui/material';
import { darkTheme } from './theme/theme';
import './plugins';
import './index.css';


const AppContainer = observer(() => {
  // const { server } = useStore();
  return (
    <ThemeProvider theme={darkTheme}>
      <CssBaseline />
      <App />
    </ThemeProvider>
  )
});

const root = ReactDOM.createRoot(
  document.getElementById('root') as HTMLElement
);
root.render(
  <React.StrictMode>
    <AppContainer />
  </React.StrictMode>
);

// If you want to start measuring performance in your app, pass a function
// to log results (for example: reportWebVitals(console.log))
// or send to an analytics endpoint. Learn more: https://bit.ly/CRA-vitals
reportWebVitals();
