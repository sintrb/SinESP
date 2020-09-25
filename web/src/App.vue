<template>
  <v-app style="height:100vh" > 
    <v-app-bar style="flex-grow:0">
      <v-app-bar-nav-icon @click="drawer = true"></v-app-bar-nav-icon>
      <v-toolbar-title>Title</v-toolbar-title>
    </v-app-bar>

    <v-navigation-drawer v-model="drawer" absolute temporary>
      <v-list nav dense>
        <v-list-item-group active-class="deep-purple--text text--accent-4">
          <v-list-item to="/">
            <v-list-item-icon>
              <v-icon>mdi-home</v-icon>
            </v-list-item-icon>
            <v-list-item-title>主页</v-list-item-title>
          </v-list-item>

          <v-list-item to="/about">
            <v-list-item-icon>
              <v-icon>mdi-heart</v-icon>
            </v-list-item-icon>
            <v-list-item-title>关于</v-list-item-title>
          </v-list-item>
        </v-list-item-group>
      </v-list>
    </v-navigation-drawer>

    <v-main>
      <v-container fluid>
        <router-view></router-view>
      </v-container>
    </v-main>
    <v-footer>
      <div
        v-if="status"
      >IDF_VER:{{status.idf_ver}} Cores:{{status.cores}} Free:{{Math.floor(status.memory/1024)}}KB Req:{{status.request}}</div>
    </v-footer>
  </v-app>
</template>

<script>
export default {
  name: "App",
  data() {
    return {
      drawer: null,
    };
  },
  computed: {
    status() {
      return this.$store.getters.status;
    },
  },
  mounted() {
    let up = () => {
      this.$store.dispatch("update_status").then(function () {
        setTimeout(up, 3000);
      });
    }
    up()
  },
};
</script>
<style lang="sass">
.flex-grow1
  flex-grow: 1
</style>