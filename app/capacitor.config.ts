import { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
  appId: 'com.ambiq.physiokit',
  appName: 'physiokit',
  webDir: 'build',
  server: {
    androidScheme: 'https'
  }
};

export default config;
