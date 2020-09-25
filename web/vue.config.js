module.exports = {
  "transpileDependencies": [
    "vuetify"
  ],
  devServer: {
    proxy: {
      '/api': {
        target: 'http://172.16.1.223',
        changeOrigin: false,
        ws: true
      }
    }
  },
}