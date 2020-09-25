// import Vuex from 'vuex'
import axios from 'axios'

let $callapi = async function (api, data) {
    let url = '/api/' + api;
    let d;
    try {
        let r = await axios.post(url, data)
        d = r.data
    }
    catch (e) {
        console.error(e)
    }
    // console.log(d)
    return d;
}
export {
    $callapi
}
export default {
    install(Vue) {
        Vue.prototype.$callapi = $callapi
        Vue.prototype.$toast = function(text){
            console.log(text)
        }
    }
};