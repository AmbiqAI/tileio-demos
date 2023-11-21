import { red } from "@mui/material/colors";
import { createTheme } from "@mui/material/styles";

export const darkTheme = createTheme({
    spacing: 8,
    palette: {
        mode: 'dark',
        primary: {
            main: '#ce6cff'
        },
        secondary: {
            main: '#11acd5'
        },
        success: {
            main: '#2fdf75'
        },
        error: {
            main: red.A400
        },
        text: {
        }
    }
});
