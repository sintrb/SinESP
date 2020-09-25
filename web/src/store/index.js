import Vue from 'vue'
import Vuex from 'vuex'
import {$callapi} from '@/base'

Vue.use(Vuex)

export default new Vuex.Store({
  state: {
    status: null
  },
  getters: {
    status: state => state.status,
  },
  mutations: {
    update_status(state, v) {
      // console.log(v)
      state.status = v
    }
  },
  actions: {
    update_status({ commit }) {
      $callapi("getStatus")
        .then(data => {
          commit('update_status', data.data)
        })
        .catch(error => {
          console.log(error)
        })
    }
  }
})
