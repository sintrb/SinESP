<template>
  <v-container>
    <v-row v-if="forms">
      <v-col v-for="d in forms" :key="d.key" cols="12">
        <v-card>
          <v-card-title>
            <span style="flex-grow:1">{{d.title}}</span>
            <v-switch v-model="d.enable"></v-switch>
          </v-card-title>
          <v-form style="padding:5px 10px" v-if="d.enable">
            <template v-for="f in d.fields">
              <v-slider
                v-if="f.type == 'slider'"
                :key="f.field"
                :label="f.title"
                :thumb-size="28"
                thumb-label="always"
                v-model="f.value"
                :min="f.min || 0"
                :max="f.max || 1000"
              ></v-slider>
              <v-text-field
                v-else
                :type="f.type && typeMap[f.type] || 'text'"
                :key="f.field"
                :label="f.title"
                v-model="f.value"
              ></v-text-field>
            </template>
          </v-form>
        </v-card>
      </v-col>
    </v-row>
    <!-- {{formdata}} -->
    <v-fab-transition>
      <v-btn color="blue" dark fixed bottom right fab @click="setConfig" :loading="doing">
        <v-icon>mdi-upload</v-icon>
      </v-btn>
    </v-fab-transition>
  </v-container>
</template>

<script>
export default {
  data: () => ({
    forms: null,
    doing: false,
    typeMap: {
      int: "number",
      float: "number",
    },
    defines: [
      {
        title: "WiFi-AP",
        key: "ap",
        fields: [
          { field: "ssid", title: "SSID", default: "ESPWeb" },
          { field: "pswd", title: "密码" },
        ],
      },
      {
        title: "WiFi-STA",
        key: "wifi",
        fields: [
          { field: "ssid", title: "SSID" },
          { field: "pswd", title: "密码" },
        ],
      },
      {
        title: "MQTT",
        key: "mqtt",
        fields: [
          { field: "host", title: "主机" },
          { field: "port", title: "端口", default: 1883, type: "int" },
          { field: "user", title: "用户名" },
          { field: "pswd", title: "密码" },
          { field: "topic", title: "主题" },
        ],
      },
      {
        title: "串口1",
        key: "uart1",
        fields: [
          { field: "baud", title: "波特率", default: 9600, type: "int" },
        ],
      },
      {
        title: "LED",
        key: "led",
        fields: [
          { field: "on", title: "亮", default: 1000, type: "slider", max: 5000 },
          { field: "off", title: "灭", default: 1000, type: "slider", max: 5000 },
        ],
      },
    ],
  }),
  computed: {
    formdata() {
      let d = this.forms ? {} : null;
      if (this.forms) {
        this.forms.map((fm) => {
          let vd = {};
          fm.fields.map((fd) => {
            var v = fd.value;
            if (fd.type == "number") v = parseInt(v);
            if (fd.type == "float") v = parseFloat(v);
            vd[fd.field] = v;
          });
          vd.enable = fm.enable ? true : false;
          d[fm.key] = vd;
        });
      }
      return d;
    },
  },
  async mounted() {
    this.doing = true;
    let r = await this.$callapi("getConfig");
    this.doing = false;
    let d = (r && r.data) || {};
    let forms = [];
    this.defines.map((df) => {
      let fm = JSON.parse(JSON.stringify(df));
      let vd = d[fm.key] || {};
      fm.fields.map((fd) => {
        let rv = vd[fd.field];
        if (rv === undefined) rv = fd.default || "";
        fd.value = rv;
      });
      fm.enable =
        vd.enable || (vd.enable === undefined && df.enable) ? true : false;
      forms.push(fm);
    });
    this.forms = forms;
  },
  methods: {
    async setConfig() {
      // 保存
      let data = this.formdata;
      if (data) {
        this.doing = true;
        let r = await this.$callapi("setConfig", this.formdata);
        this.doing = false;
        if (r && r.data) {
          this.$toast("保存成功!");
        }
      }
    },
  },
};
</script>
