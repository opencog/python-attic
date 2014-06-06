__author__ = 'Cosmo Harrigan'

from flask import Flask
from flask.ext.restful import Api
from apiatomcollection import *
from apitypes import *
from apishell import *


class RESTAPI(object):
    """
    REST API for OpenCog

    Implemented using the Flask micro-framework and Flask-RESTful extension

    Documentation:
    http://wiki.opencog.org/w/REST_API

    Prerequisites:
    Flask, mock, flask-restful, six

    Default endpoint: http://127.0.0.1:5000/api/v1.1/
    (Replace 127.0.0.1 with the IP address of the server if necessary)

    Example request: http://127.0.0.1:5000/api/v1.1/atoms?type=ConceptNode

    See: opencog/python/web/api/exampleclient.py for detailed examples of usage, and review
    the method definitions in each resource for request/response specifications.
    """

    def __init__(self, atomspace):
        self.atomspace = atomspace

        # Initialize the web server and set the routing
        self.app = Flask(__name__, static_url_path="")
        self.api = Api(self.app)
        atom_collection_api = AtomCollectionAPI.new(self.atomspace)
        atom_types_api = TypesAPI()
        shell_api = ShellAPI()
        self.api.add_resource(atom_collection_api, '/api/v1.1/atoms', '/api/v1.1/atoms/<int:id>', endpoint='atoms')
        self.api.add_resource(atom_types_api, '/api/v1.1/types', endpoint='types')
        self.api.add_resource(shell_api, '/api/v1.1/shell', endpoint='shell')

    def run(self, host='127.0.0.1', port=5000):
        """
        Runs the REST API

        :param host: the hostname to listen on. Set this to ``'0.0.0.0'`` to
                     have the server available externally as well. Defaults to
                     ``'127.0.0.1'``.
        :param port: the port of the webserver. Defaults to ``5000``
        """
        self.app.run(debug=False, host=host, port=port)

    def test(self):
        """
        Returns a test client for the REST API
        """
        return self.app.test_client()

