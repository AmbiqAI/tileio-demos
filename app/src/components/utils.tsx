import styled from '@emotion/styled';
import { keyframes } from '@emotion/react';

export const GridContainer = styled.div`
  width: 100%;
  height: 100%;
  position: relative;
`;

export const GridZStack = styled('div')(({ level }: {level: number}) => ({
  'position': 'absolute',
  'top': 0,
  'left': 0,
  'bottom': 0,
  'right': 0,
  zIndex: level ? level+90 : 0
}));


const pulse = keyframes`
0% {
  transform: scale(0.9);
}

40% {
  transform: scale(0.9);
}

60% {
  transform: scale(1.1);
}

65% {
  transform: scale(0.9);
}

70% {
  transform: scale(1.1);
}

90% {
  transform: scale(0.9);
}

100% {
  transform: scale(0.9);
}
`

export const PulsedDiv =styled.div`
  animation: ${pulse} 1s ease infinite;
`;

export function getMinMax(array: number[], minThreshold?: number): [number, number] {
  let min = array[0], max = array[0];
  for (let i = 1; i < array.length; i++) {
    let value = array[i];
    if (minThreshold !== undefined && value < minThreshold) { continue; }
    min = (value < min) ? value : min;
    max = (value > max) ? value : max;
  }
  return [min, max];
}
