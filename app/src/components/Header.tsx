import { AppBar } from '@mui/material';
import { Box } from '@mui/system';
import React from 'react';

interface Props {
  children?: React.ReactNode
}

function Header({ children }: Props) {
  return (
    <Box sx={{ flexGrow: 1, mb: 5 }}>
      <AppBar
        position='fixed' color='inherit'
        sx={{position: 'fixed', pt: 1, top: 0, bottom: 'auto', left: 0, right: 0}}
      >
        {children}
      </AppBar>
    </Box>
  )
}

export default Header;