(function($) {

  $.fn.onSingle = function(events, callback){
    if ("function" == typeof(callback)) {
        var t = this;
        var internalCallback = function(event){
            $(t).off(events, internalCallback);
            callback.apply(t, [event]);
            setTimeout(function(){
                $(t).on(events, internalCallback);
            }, 0);
        };
        $(t).on(events, internalCallback);
    }
    return $(this);
  };

  window.History = window.History || {};

  // Initializes history object.
  History.init = function() {
    var self = this;

    // Whether HTML5 state is supported or not.
    self.useState = (history.pushState !== undefined);

    // Store current state and hash.
    self.currentState = self._state();
    self.currentHash = window.location.hash;

    // Get the triggers of hashchange event and popstate event.
    // We use onSingle to run once for both events.
    $(window).onSingle('hashchange popstate', function(e) {
      if (self.currentState != self._state()) {
        self.currentState = self._state();
        $(window).trigger('history:statechange');
      }
      else {
        self.currentHash = window.location.hash;
        if (self.getHash().indexOf('/') === 0) {
          $(window).trigger('history:statechange');
        }
        else {
          $(window).trigger('history:hashchange');
        }
      }
    });

    // Triget the hashchange on load.
    if (self.currentHash !== '') {
      $(window).trigger('hashchange');
    }
  };

  // Reset the current state from current location.
  History._state = function(url) {
    var myURL = new Url(url || window.location.href);
    return myURL.path + (myURL.query.toString() ? '?' + myURL.query : '');
  };

  // Returns the current state.
  History.getState = function() {
    return this.useState ? this.currentState : this.getHash();
  };

  // Returns the current hash.
  History.getHash = function() {
    return this.currentHash.replace('#', '');
  };

  // Escapes the current hash.
  History.escapeHash = function(value) {
    return window.encodeURIComponent(value);
  };

  // Unescapes the current hash.
  History.unescapeHash = function(value) {
    return window.decodeURIComponent(value);
  };

  // Return the url object for state.
  History.URL = function () {
    return new Url(History.getState());
  };

  // Exposes the hash API.
  History.hash = function(value) {
    if (this.useState && value === '') {
      return this.state(this.currentState);
    }

    // If value is URL then cleanup url
    // from possible hash values.
    if (value.indexOf('http') === 0) {
      value = this._state(value);
    }

    window.location.hash = value;
  };

  // Exposes the state API.
  History.state = function(url) {
    // If HTML state exists then prefer it
    // else use the legacy hash functionality.
    if (this.useState) {
      history.pushState(null, null, this._state(url));
      $(window).trigger('popstate');
    }
    else {
      this.hash(this._state(url));
    }
  };

  // Initializes the History plugin.
  $(document).ready(function() {
    History.init();
  });

})(jQuery);
